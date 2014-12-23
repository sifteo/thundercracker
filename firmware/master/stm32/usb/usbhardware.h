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

#ifndef USB_HARDWARE_H
#define USB_HARDWARE_H

#include "usb/usbcore.h"

#include <stdint.h>

/*
 * Platform specific interface to perform hardware related USB tasks.
 */
namespace UsbHardware
{
    static const unsigned MAX_PACKET = 64;

    void init(const UsbCore::Config &cfg);
    void deinit();
    void reset();

    void setAddress(uint8_t addr);
    void epINSetup(uint8_t addr, uint8_t type, uint16_t max_size);
    void epOUTSetup(uint8_t addr, uint8_t type, uint16_t max_size);

    void epStall(uint8_t addr);
    void epClearStall(uint8_t addr);
    bool epIsStalled(uint8_t addr);

    uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len);
    uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len);

    void disconnect();
}

#endif // USB_HARDWARE_H
