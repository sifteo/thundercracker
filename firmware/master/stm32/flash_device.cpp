/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "flash_device.h"
#include "macronixmx25.h"
#include "board.h"

static MacronixMX25 flash(FLASH_CS_GPIO,
                          SPIMaster(&FLASH_SPI,
                                    FLASH_SCK_GPIO,
                                    FLASH_MISO_GPIO,
                                    FLASH_MOSI_GPIO,
                                    MacronixMX25::dmaCompletionCallback));

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
