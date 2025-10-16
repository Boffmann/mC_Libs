#include "espinputoutput.h"
#include <Arduino.h>

namespace jellED {

BinaryState EspInputOutput::digitalReadPin(uint8_t pin) {
    if (digitalRead(pin) == 0) {
        return STATE_LOW;
    }
    return STATE_HIGH;
}

} // namespace jellED