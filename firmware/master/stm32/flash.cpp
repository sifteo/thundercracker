
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

bool Flash::write(uint32_t address, const uint8_t *buf, unsigned len) {
    return flash.write(address, buf, len) == MacronixMX25::Ok;
}

bool Flash::eraseSector(uint32_t address) {
    return flash.eraseSector(address) == MacronixMX25::Ok;
}

bool Flash::chipErase() {
    return flash.chipErase() == MacronixMX25::Ok;
}

