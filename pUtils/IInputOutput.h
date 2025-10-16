#ifndef __JELLED_I_INPUTOUTPUT_H__
#define __JELLED_I_INPUTOUTPUT_H__

#include <stdint.h>

namespace jellED {

enum BinaryState {
    STATE_HIGH,
    STATE_LOW
};

class IInputOutput {
public:
    virtual ~IInputOutput() = default;
    virtual BinaryState digitalReadPin(uint8_t pin) = 0;
};

} // namespace jellED

#endif
