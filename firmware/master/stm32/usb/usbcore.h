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

#ifndef USBCORE_H
#define USBCORE_H

#include "usb/usbdefs.h"

class UsbCore
{
public:

    struct Config {
        bool enableSOF;
    };

    static void init(const Usb::DeviceDescriptor *dev,
                     const Usb::ConfigDescriptor *conf,
                     const Usb::WinUsbCompatIdHeaderDescriptor *win,
                     const Config & cfg);
    static void reset();

    // control requests
    static void setup();
    static void out();
    static void in();

private:
    static const Usb::DeviceDescriptor *devDesc;
    static const Usb::ConfigDescriptor *configDesc;
    static const Usb::WinUsbCompatIdHeaderDescriptor *winCompatDesc;

    static uint16_t address;
    static uint16_t _config;

    static bool setupHandler();
    static void setErrorState();

    enum ControlStatus {
        Idle,
        Stalled,
        DataIn,
        LastDataIn,
        StatusIn,
        DataOut,
        LastDataOut,
        StatusOut
    };

    enum DeviceState {
        Uninit      = 0,
        Stopped     = 1,
        Ready       = 2,
        Selected    = 3,
        Active      = 4
    };

    enum Ep0State {
        EP0_WAITING_SETUP,
        EP0_TX,
        EP0_WAITING_TX0,
        EP0_WAITING_STS,
        EP0_RX,
        EP0_SENDING_STS,
        EP0_ERROR
    };

    struct ControlState {
        Ep0State ep0Status;
        DeviceState state;
        uint16_t status;
        uint8_t configuration;
        Usb::SetupData req;
        uint8_t *pdata;
        uint16_t len;

        void ALWAYS_INLINE setupTransfer(const void *b, uint16_t sz) {
            len = sz;
            pdata = (uint8_t*)b;
        }
    };
    static ControlState controlState;
};

#endif // USBCORE_H
