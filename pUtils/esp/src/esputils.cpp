#include "esputils.h"

namespace jellED {

EspPlatformUtils::EspPlatformUtils() {
   this->_logger = EspLogger();
   this->_crono = EspCrono();
   this->_io = EspInputOutput();
}

ILogger& EspPlatformUtils::logger() {
    return this->_logger;
}

ICrono& EspPlatformUtils::crono() {
    return this->_crono;
}

IInputOutput& EspPlatformUtils::io() {
    return this->_io;
}

} // namespace jellED
