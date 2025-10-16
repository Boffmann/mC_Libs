#ifndef __JELLED_ESP_INPUTOUTPUT_H__
#define __JELLED_ESP_INPUTOUTPUT_H__

#include "../../IInputOutput.h"

namespace jellED {

class EspInputOutput : public IInputOutput {
public:
    virtual BinaryState digitalReadPin(uint8_t pin);
};

}
#endif