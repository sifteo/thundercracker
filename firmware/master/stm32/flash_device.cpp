/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_device.h"
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
 
void FlashDevice::init() {
    flash.init();
}

void FlashDevice::read(uint32_t address, uint8_t *buf, unsigned len) {
    if (len)
        flash.read(address, buf, len);
}

void FlashDevice::write(uint32_t address, const uint8_t *buf, unsigned len) {
    if (len)
        flash.write(address, buf, len);
}

void FlashDevice::eraseSector(uint32_t address) {
    flash.eraseSector(address);
}

void FlashDevice::chipErase() {
    flash.chipErase();
}

bool FlashDevice::writeInProgress() {
    return flash.writeInProgress();
}

void FlashDevice::readId(JedecID *id)
{
    flash.readId(id);
}
