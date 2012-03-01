/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash.h"
#include "macronixmx25.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    Routines to implement the Flash interface in flash.h
    based on our macronix flash part.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void Flash::init() {
    MacronixMX25::instance.init();
}

void Flash::read(uint32_t address, uint8_t *buf, unsigned len) {
    MacronixMX25::instance.read(address, buf, len);
}

bool Flash::write(uint32_t address, const uint8_t *buf, unsigned len) {
    return MacronixMX25::instance.write(address, buf, len) == MacronixMX25::Ok;
}

bool Flash::eraseSector(uint32_t address) {
    return MacronixMX25::instance.eraseSector(address) == MacronixMX25::Ok;
}

bool Flash::chipErase() {
    return MacronixMX25::instance.chipErase() == MacronixMX25::Ok;
}

