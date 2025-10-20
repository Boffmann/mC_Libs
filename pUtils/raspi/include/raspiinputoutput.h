#ifndef __JELLED_RASPI_INPUTOUTPUT_H__
#define __JELLED_RASPI_INPUTOUTPUT_H__

#include "../../IInputOutput.h"
#include <gpiod.hpp>

namespace jellED {

class RaspiInputOutput : public IInputOutput {
private:
    gpiod::chip chip;
public:
    RaspiInputOutput();
    BinaryState digitalReadPin(uint8_t pin);
};

} // namespace jellED

#endif
