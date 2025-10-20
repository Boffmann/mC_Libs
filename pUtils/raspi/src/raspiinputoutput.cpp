#include "../include/raspiinputoutput.h"
#include <gpiod.hpp>

namespace jellED {

RaspiInputOutput::RaspiInputOutput() : chip{"/dev/gpiochip0"} {}

BinaryState RaspiInputOutput::digitalReadPin(uint8_t pin) {
    try {
        auto request = this->chip.prepare_request()
            .set_consumer("gpio-reader")
            .add_line_settings(pin, gpiod::line_settings()
                .set_direction(gpiod::line::direction::INPUT))
            .do_request();
        
	auto pin_value = request.get_value(pin);

	return (pin_value == gpiod::line::value::ACTIVE) ? STATE_HIGH : STATE_LOW;
        
    } catch (const std::exception& e) {
	throw e;
    }
}

} // namespace jellED
