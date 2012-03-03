
#include "flash.h"
#include "macronixmx25.h"
#include "board.h"

static MacronixMX25 flash(SPIMaster(&FLASH_SPI,
                                    FLASH_CS_GPIO,
                                    FLASH_SCK_GPIO,
                                    FLASH_MISO_GPIO,
                                    FLASH_MOSI_GPIO));

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    Routines to implement the Flash interface in flash.h
    based on our macronix flash part.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void Flash::init() {
    flash.init();
}

void Flash::read(uint32_t address, uint8_t *buf, unsigned len) {
    flash.read(address, buf, len);
}

// TODO: remove bools from the signatures here, this is just historical & not useful
bool Flash::write(uint32_t address, const uint8_t *buf, unsigned len) {
    flash.write(address, buf, len);
    return true;
}

bool Flash::eraseSector(uint32_t address) {
    flash.eraseSector(address);
    return true;
}

bool Flash::chipErase() {
    flash.chipErase();
    return true;
}

