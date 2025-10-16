#include "../include/raspiinputoutput.h"
//#include "driver/gpio.h"

namespace jellED {

BinaryState RaspiInputOutput::digitalReadPin(uint8_t pin) {
    // if (gpio_get_level(gpio_num_t gpio_num) == 0) {
    //     return STATE_LOW;
    // }
    return STATE_HIGH;
}

} // namespace jellED
