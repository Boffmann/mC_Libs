#include "usbMicro.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdint.h>
#include <thread>
#include <unistd.h>

namespace jellED {

static void overflow_callback(struct SoundIoInStream *instream) {
    fprintf(stderr, "!!! AUDIO OVERFLOW - samples lost !!!\n");
}

#define NO_SUPPORTED_SAMPLE_RATE -1

// https://github.com/andrewrk/libsoundio/blob/master/example/sio_microphone.c
UsbMicro::UsbMicro(std::string &device_id, enum SoundIoBackend backend)
    : soundio{nullptr}, microphone{nullptr}, mic_in_stream{nullptr},
      initialization_time{std::chrono::steady_clock::now()} {
  this->device_id = device_id.c_str();
  // Initialize RecordContext
  this->rc.ring_buffer = nullptr;
  this->backend = backend;
};

UsbMicro::~UsbMicro() {
  if (mic_in_stream) {
    soundio_instream_destroy(mic_in_stream);
  }
  if (microphone) {
    soundio_device_unref(microphone);
  }
  if (soundio) {
    soundio_destroy(soundio);
  }
}

static void read_callback(struct SoundIoInStream *instream, int frame_count_min,
                          int frame_count_max) {
  struct RecordContext *rc = (RecordContext *)instream->userdata;
  struct SoundIoChannelArea *areas;
  int err;

  char *write_ptr = soundio_ring_buffer_write_ptr(rc->ring_buffer);
  int free_bytes = soundio_ring_buffer_free_count(rc->ring_buffer);
  int free_count = free_bytes / instream->bytes_per_frame;

  // FIXED: Read as many frames as we can fit, don't drop everything
  if (free_count < frame_count_min) {
    fprintf(stderr, "ring buffer overflow - dropping %d frames\n", frame_count_min);
    // Must still call begin_read/end_read to acknowledge the audio!
    int frame_count = frame_count_min;
    if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
      return;
    }
    if (frame_count > 0) {
      soundio_instream_end_read(instream);  // Discard, but don't block the stream
    }
    return;
  }

  int write_frames = std::min(free_count, frame_count_max);
  int frames_left = write_frames;
  int total_written = 0;

  while (frames_left > 0) {
    int frame_count = frames_left;

    if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
      fprintf(stderr, "begin read error: %s\n", soundio_strerror(err));
      break;
    }

    if (frame_count == 0) {
      break;
    }

    if (!areas) {
      // Hole in stream - fill with silence
      memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
      write_ptr += frame_count * instream->bytes_per_frame;
    } else {
      // OPTIMIZED: For mono streams, use single memcpy per chunk
      if (instream->layout.channel_count == 1) {
        // Mono: direct copy if contiguous
        if (areas[0].step == instream->bytes_per_sample) {
          memcpy(write_ptr, areas[0].ptr, frame_count * instream->bytes_per_sample);
          write_ptr += frame_count * instream->bytes_per_sample;
        } else {
          // Non-contiguous mono (rare)
          for (int frame = 0; frame < frame_count; frame++) {
            memcpy(write_ptr, areas[0].ptr, instream->bytes_per_sample);
            areas[0].ptr += areas[0].step;
            write_ptr += instream->bytes_per_sample;
          }
        }
      } else {
        // Multi-channel: interleave (original logic)
        for (int frame = 0; frame < frame_count; frame++) {
          for (int ch = 0; ch < instream->layout.channel_count; ch++) {
            memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
            areas[ch].ptr += areas[ch].step;
            write_ptr += instream->bytes_per_sample;
          }
        }
      }
    }

    if ((err = soundio_instream_end_read(instream))) {
      fprintf(stderr, "end read error: %s\n", soundio_strerror(err));
      break;
    }

    total_written += frame_count;
    frames_left -= frame_count;
  }

  soundio_ring_buffer_advance_write_ptr(rc->ring_buffer,
                                        total_written * instream->bytes_per_frame);
}

SoundIoFormat find_supported_format(SoundIoDevice *microphone) {
  // Try to find a supported format
  SoundIoFormat supported_format;
  SoundIoFormat formats[] = {SoundIoFormatS16NE, SoundIoFormatS24NE,
                             SoundIoFormatS32NE, SoundIoFormatFloat32NE};
  bool format_found = false;
  for (int i = 0; i < 4; i++) {
    if (soundio_device_supports_format(microphone, formats[i])) {
      supported_format = formats[i];
      format_found = true;
      std::cout << "Using format: " << soundio_format_string(supported_format)
                << std::endl;
      break;
    }
  }
  if (!format_found) {
    return SoundIoFormatInvalid;
  }
  return supported_format;
}

int find_supported_sample_rate(SoundIoDevice *microphone) {
  // Try common sample rates
  int rates[] = {8000, 16000, 22050, 44100, 48000};
  int supported_sample_rate;
  bool rate_found = false;
  for (int i = 0; i < 4; i++) {
    if (soundio_device_supports_sample_rate(microphone, rates[i])) {
      supported_sample_rate = rates[i];
      rate_found = true;
      std::cout << "Using sample rate: " << supported_sample_rate << std::endl;
      break;
    }
  }
  if (!rate_found) {
    return NO_SUPPORTED_SAMPLE_RATE;
  }

  return supported_sample_rate;
}

void UsbMicro::initialize() {
  bool in_raw = false;
  int sample_rate = 48000;
  SoundIoFormat prioritized_format = SoundIoFormatS16NE;

  this->soundio = soundio_create();
  if (!soundio) {
    std::cout << "out of memory" << std::endl;
    return;
  }

  // int err = soundio_connect(soundio);
  int err = soundio_connect_backend(soundio, this->backend);
  if (err) {
    std::cout << "error connecting " << soundio_strerror(err) << std::endl;
    return;
  }
  soundio_flush_events(soundio);

  int input_device_count = soundio_input_device_count(soundio);
  if (input_device_count < 0) {
    std::cout << "no input device found" << std::endl;
    return;
  }

  // Try to connect to input device_id
  int in_device_index = 0;
  bool found = false;
  for (int i = 0; i < input_device_count; i += 1) {
    struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
    if (device->is_raw == in_raw && strcmp(device->id, this->device_id) == 0) {
      std::cout << "Found device: " << device->name << std::endl;
      in_device_index = i;
      found = true;
      soundio_device_unref(device);
      break;
    } else {
      std::cout << "Device: " << device->name << " is not found" << std::endl;
    }
    soundio_device_unref(device);
  }
  if (!found) {
    std::cout << "invalid input device id " << this->device_id << std::endl;
    return;
  }

  microphone = soundio_get_input_device(soundio, in_device_index);
  if (!microphone) {
    std::cout << "could not get input device: out of memory" << std::endl;
    return;
  }
  std::cout << "Got in device: " << microphone->name << std::endl;

  if (microphone->probe_error) {
    fprintf(stderr, "Unable to probe device: %s\n",
            soundio_strerror(microphone->probe_error));
    return;
  }

  soundio_device_sort_channel_layouts(microphone);

  // Check format compatibility and fallback if needed
  if (!soundio_device_supports_format(microphone, prioritized_format)) {
    std::cout << "microphone does not support format "
              << soundio_format_string(prioritized_format) << std::endl;
    prioritized_format = find_supported_format(this->microphone);
    if (prioritized_format == SoundIoFormatInvalid) {
      std::cout << "No supported format found" << std::endl;
      return;
    }
  }

  if (!soundio_device_supports_sample_rate(microphone, sample_rate)) {
    std::cout << "microphone does not support sample rate " << sample_rate
              << std::endl;
    sample_rate = find_supported_sample_rate(this->microphone);
    if (sample_rate == NO_SUPPORTED_SAMPLE_RATE) {
      std::cout << "No supported sample rate found" << std::endl;
      return;
    }
  }

  mic_in_stream = soundio_instream_create(microphone);
  if (!mic_in_stream) {
    std::cout << "out of memory instream" << std::endl;
    return;
  }
  mic_in_stream->format = prioritized_format;
  mic_in_stream->sample_rate = sample_rate;
  mic_in_stream->read_callback = &read_callback;
  mic_in_stream->userdata = &this->rc;
  mic_in_stream->software_latency = 0.02;

  mic_in_stream->overflow_callback = &overflow_callback;

  if ((err = soundio_instream_open(mic_in_stream))) {
    fprintf(stderr, "unable to open input stream: %s", soundio_strerror(err));
    return;
  }

  fprintf(stderr, "%s %dHz %s interleaved\n", mic_in_stream->layout.name,
          sample_rate, soundio_format_string(prioritized_format));

  const int ring_buffer_duration_seconds = 1;
  int capacity = ring_buffer_duration_seconds * mic_in_stream->sample_rate *
                 mic_in_stream->bytes_per_frame;
  std::cout << "capacity: " << capacity << std::endl;
  rc.ring_buffer = soundio_ring_buffer_create(soundio, capacity);
  if (!rc.ring_buffer) {
    fprintf(stderr, "out of memory\n");
    return;
  }

  if ((err = soundio_instream_start(mic_in_stream))) {
    std::cout << "unable to start input device: " << soundio_strerror(err)
              << std::endl;
    return;
  }
}

bool UsbMicro::read(AudioBuffer *buffer) {
  // DON'T call soundio_flush_events() here - it's expensive!
  // The callback handles events asynchronously
  
  if (!mic_in_stream || !rc.ring_buffer) {
    return false;
  }

  int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
  if (fill_bytes == 0) {
    buffer->num_samples = 0;
    return false;  // No data available
  }
  
  char *read_buf = soundio_ring_buffer_read_ptr(rc.ring_buffer);
  int bytes_per_sample = mic_in_stream->bytes_per_sample;
  int samples_available = fill_bytes / bytes_per_sample;
  int samples_to_read = std::min(samples_available, (int)I2S_DMA_BUF_LEN);

  buffer->num_samples = samples_to_read;
  buffer->bytes_read = samples_to_read * bytes_per_sample;
  buffer->samplingRate = mic_in_stream->sample_rate;

  // OPTIMIZED: Format-specific conversion without switch in loop
  // Pre-compute normalization factor (multiply is faster than divide)
  if (mic_in_stream->format == SoundIoFormatS16NE) {
    // Most common case - optimized path
    const int16_t* src = reinterpret_cast<const int16_t*>(read_buf);
    constexpr double norm = 1.0 / 32768.0;
    for (int i = 0; i < samples_to_read; i++) {
      buffer->buffer[i] = src[i] * norm;
    }
  } else if (mic_in_stream->format == SoundIoFormatFloat32NE) {
    const float* src = reinterpret_cast<const float*>(read_buf);
    for (int i = 0; i < samples_to_read; i++) {
      buffer->buffer[i] = src[i];
    }
  } else if (mic_in_stream->format == SoundIoFormatS32NE) {
    const int32_t* src = reinterpret_cast<const int32_t*>(read_buf);
    constexpr double norm = 1.0 / 2147483648.0;
    for (int i = 0; i < samples_to_read; i++) {
      buffer->buffer[i] = src[i] * norm;
    }
  } else if (mic_in_stream->format == SoundIoFormatS24NE) {
    // 24-bit is trickier, keep original logic but outside switch
    constexpr double norm = 1.0 / 8388608.0;
    for (int i = 0; i < samples_to_read; i++) {
      int32_t sample = 0;
      memcpy(&sample, read_buf + (i * bytes_per_sample), 3);
      if (sample & 0x800000) {
        sample |= 0xFF000000;
      }
      buffer->buffer[i] = sample * norm;
    }
  }

  soundio_ring_buffer_advance_read_ptr(rc.ring_buffer,
                                       samples_to_read * bytes_per_sample);
  return true;
}

void UsbMicro::drain_audio_buffer() {
    std::cout << "Draining old audio from buffer..." << std::endl;
    while (true) {
        int pending = soundio_ring_buffer_fill_count(rc.ring_buffer);
        if (pending < mic_in_stream->bytes_per_sample * 100) {  // < 100 samples
            break;  // Buffer is nearly empty, start processing
        }
        // Advance read pointer to discard old audio
        soundio_ring_buffer_advance_read_ptr(rc.ring_buffer, pending);
        soundio_flush_events(soundio);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void UsbMicro::print_available_input_devices(enum SoundIoBackend backend) {
  struct SoundIo *soundio = soundio_create();
  if (!soundio) {
    std::cout << "out of memory" << std::endl;
    return;
  }
  int count = soundio_backend_count(soundio);
  for (int i = 0; i < count; i++) {
    enum SoundIoBackend backend = soundio_get_backend(soundio, i);
    printf("Available backend: %s\n", soundio_backend_name(backend));
  }
  int err = soundio_connect_backend(soundio, backend);

  if (err) {
    std::cout << "error connecting: " << soundio_strerror(err) << std::endl;
    soundio_destroy(soundio);
    return;
  }
  soundio_flush_events(soundio);
  printf("Using sound Backend: %s\n",
         soundio_backend_name(soundio->current_backend));
  int num_devices = soundio_input_device_count(soundio);
  std::cout << "Available input devices (" << num_devices << "):" << std::endl;
  for (int i = 0; i < num_devices; ++i) {
    SoundIoDevice *dev = soundio_get_input_device(soundio, i);
    std::cout << "  [" << i << "] Name: '" << dev->name << "', ID: '" << dev->id
              << "'" << std::endl;
    soundio_device_unref(dev);
  }
  soundio_destroy(soundio);
}

int UsbMicro::getSampleRate() { return this->mic_in_stream->sample_rate; }

int UsbMicro::getPendingSampleCount() {
    if (!rc.ring_buffer || !mic_in_stream) return 0;
    int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
    return fill_bytes / mic_in_stream->bytes_per_sample;
}

} // namespace jellED
