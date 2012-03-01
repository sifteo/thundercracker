/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

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

bool Flash::write(uint32_t address, const uint8_t *buf, unsigned len) {
    return flash.write(address, buf, len);
}

bool Flash::eraseSector(uint32_t address) {
    return flash.eraseSector(address);
}

bool Flash::chipErase() {
    return flash.chipErase();
}

bool Flash::flush() {
    return flash.flush();
}
