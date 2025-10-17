#include "pushbutton.h"

namespace jellED {

PushButton::PushButton(IPlatformUtils& platformUtils, uint8_t dataPin)
: platformUtils(platformUtils),
    dataPin{dataPin},
    buttonStates{0},
    T1{0},
    T2{0} {}

bool PushButton::buttonScan() {
    int sumStates = 0;
    this->buttonStates <<= 1;
    this->buttonStates |= (platformUtils.io().digitalReadPin(this->dataPin) == STATE_HIGH) ? 1 : 0;
    sumStates = __builtin_popcount(this->buttonStates);
    if (sumStates >= 10) {
        return true;
    }
    return false;
}

bool PushButton::isPressed() {
    this->T2 = this->platformUtils.crono().currentTimeMicros();
    if ((this->T2 - this->T1) > 5) {
        bool result = this->buttonScan();
        this->T1 = this->platformUtils.crono().currentTimeMicros();
        return result;
    }
    return false;
}

} // namespace jellED
