#include "../include/raspiinputoutput.h"
#include "gpiod.hpp"

namespace jellED {

BinaryState RaspiInputOutput::digitalReadPin(uint8_t pin) {
    gpiod::chip chip("gpiochip0");

    gpiod::line line = chip.get_line(pin);

    // Request it as input
    line.request({"gpio-reader", gpiod::line_request::DIRECTION_INPUT});

    // Read its value
    int value = line.get_value();
    if (value == 0) {
        return STATE_LOW;
    }
    return STATE_HIGH;
}

} // namespace jellED
