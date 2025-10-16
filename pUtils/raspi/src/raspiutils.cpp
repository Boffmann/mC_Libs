#include "../include/raspiutils.h"

namespace jellED {

RaspiPlatformUtils::RaspiPlatformUtils() {
    this->_logger = RaspiLogger();
    this->_crono = RaspiCrono();
    this->_io = RaspiInputOutput();
}

ILogger& RaspiPlatformUtils::logger() {
    return this->_logger;
}

ICrono& RaspiPlatformUtils::crono() {
    return this->_crono;
}

IInputOutput& RaspiPlatformUtils::io() {
    return this->_io;
}

} // namespace jellED
