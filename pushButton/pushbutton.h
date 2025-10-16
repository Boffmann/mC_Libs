#ifndef _PUSHBUTTON_H_
#define _PUSHBUTTON_H_

#include <stdint.h>
#include "../pUtils/IPlatformUtils.h"

namespace jellED {

class PushButton {
private:
    uint8_t dataPin;
    IPlatformUtils& platformUtils;
    uint16_t buttonStates;
    unsigned long T1, T2;

    bool buttonScan();

public:
    PushButton(IPlatformUtils& platformUtils, uint8_t dataPin);

    bool isPressed();
};

} // namespace jellED

#endif