#ifndef __JELLED_RASPI_INPUTOUTPUT_H__
#define __JELLED_RASPI_INPUTOUTPUT_H__

#include "../../IInputOutput.h"

namespace jellED {

class RaspiInputOutput : public IInputOutput {
public:
    BinaryState digitalReadPin(uint8_t pin);
};

} // namespace jellED

#endif
