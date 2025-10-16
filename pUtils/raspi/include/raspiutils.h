#ifndef __JELLED_I_RASPI_PLATFORMUTILS_H__
#define __JELLED_I_RASPI_PLATFORMUTILS_H__

#include "../../IPlatformUtils.h"

#include "raspilogger.h"
#include "raspicrono.h"
#include "raspiinputoutput.h"

namespace jellED {

class RaspiPlatformUtils : public IPlatformUtils {
private:
    RaspiLogger _logger;
    RaspiCrono _crono;
    RaspiInputOutput _io;

public:
    RaspiPlatformUtils();
    ILogger& logger();
    ICrono& crono();
    IInputOutput& io();
};

} // namespace jellED

#endif
