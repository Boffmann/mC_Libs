#ifndef __JELLED_I_ESP_PLATFORMUTILS_H__
#define __JELLED_I_ESP_PLATFORMUTILS_H__

#include "../../IPlatformUtils.h"

#include "esplogger.h"
#include "espcrono.h"
#include "espinputoutput.h"

namespace jellED {

class EspPlatformUtils : public IPlatformUtils {
private:
    EspLogger _logger;
    EspCrono _crono;
    EspInputOutput _io;

public:
    EspPlatformUtils();
    ILogger& logger();
    ICrono& crono();
    IInputOutput& io();
};

} // namespace jellED

#endif
