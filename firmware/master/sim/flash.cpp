
#include "flash.h"
#include "flashstorage.h"

static FlashStorage flash;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    Routines to implement the Flash interface in flash.h
    based on a file.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void Flash::init() {
    flash.init();
}

void Flash::read(uint32_t address, uint8_t *buf, unsigned len) {
    flash.read(address, buf, len);
}

void Flash::write(uint32_t address, const uint8_t *buf, unsigned len) {
    flash.write(address, buf, len);
}

void Flash::eraseSector(uint32_t address) {
    flash.eraseSector(address);
}

void Flash::chipErase() {
    flash.chipErase();
}

void Flash::flush() {
    flash.flush();
}

bool Flash::writeInProgress() {
    // this is never async in the simulator
    return false;
}
