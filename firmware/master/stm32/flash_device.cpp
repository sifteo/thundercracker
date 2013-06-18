/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_device.h"
#include "flash_map.h"
#include "nor_spi.h"
#include "board.h"

static NorSpi flash(FLASH_CS_GPIO,
                          SPIMaster(&FLASH_SPI,
                                    FLASH_SCK_GPIO,
                                    FLASH_MISO_GPIO,
                                    FLASH_MOSI_GPIO,
                                    NorSpi::dmaCompletionCallback));

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    Routines to implement the Flash interface in flash.h
    based on our macronix flash part.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
unsigned FlashDevice::capacity() {
    STATIC_ASSERT(MAP_BLOCK_SIZE == FlashMapBlock::BLOCK_SIZE);
    return MIN(NorSpi::CAPACITY, MAX_CAPACITY);
}

uint8_t FlashDevice::mfgr_id() {
    return NorSpi::MFGR_ID;
}

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

void FlashDevice::eraseBlock(uint32_t address) {
    flash.eraseBlock(address);
}

void FlashDevice::eraseAll() {
    flash.chipErase();
}

bool FlashDevice::busy() {
    return flash.busy();
}

void FlashDevice::readId(JedecID *id)
{
    flash.readId(id);
}
