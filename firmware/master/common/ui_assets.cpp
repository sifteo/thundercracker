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

#include "ui_assets.h"

extern const uint16_t v01_LogoWhiteOnBlue_data[];
extern const uint16_t v01_MenuBackground_data[];
extern const uint16_t v01_IconQuit_data[];
extern const uint16_t v01_IconBack_data[];
extern const uint16_t v01_IconResume_data[];
extern const uint16_t v01_ShutdownBackground_data[];

extern const uint16_t v02_LogoWhiteOnBlue_data[];
extern const uint16_t v02_MenuBackground_data[];
extern const uint16_t v02_IconQuit_data[];
extern const uint16_t v02_IconBack_data[];
extern const uint16_t v02_IconResume_data[];
extern const uint16_t v02_ShutdownBackground_data[];
extern const uint16_t v02_BigDigits_data[];
extern const uint16_t v02_LowBattBase_data[];
extern const uint16_t v02_FaultMessage_data[];
extern const uint16_t v02_CubeRangePause_data[];
extern const uint16_t v02_BluetoothPairing_data[];


void UIAssets::init(unsigned version)
{
    switch (version) {

        case 0x01:
            logoWhiteOnBlue     = v01_LogoWhiteOnBlue_data;
            menuBackground      = v01_MenuBackground_data;
            iconQuit            = v01_IconQuit_data;
            iconBack            = v01_IconBack_data;
            iconResume          = v01_IconResume_data;
            shutdownBackground  = v01_ShutdownBackground_data;
            bigDigits           = v02_BigDigits_data;           // XXX - No v01 equivalent yet
            faultMessage        = v02_FaultMessage_data;        // XXX - No v01 equivalent yet
            iconCubeRange       = v02_CubeRangePause_data;      // XXX - No v01 equivalent yet
            iconLowBattBase     = v02_LowBattBase_data;         // XXX - No v01 equivalent yet
            bluetoothPairing    = v02_BluetoothPairing_data;    // XXX - No v01 equivalent yet
            menuHeight          = 9;
            menuTextPalette     = 11;
            iconSize            = 5;
            iconSpacing         = 8;
            shutdownHeight      = 10;
            shutdownY1          = 1;
            shutdownY2          = 8;
            break;

        default:
        case 0x02:
            logoWhiteOnBlue     = v02_LogoWhiteOnBlue_data;
            menuBackground      = v02_MenuBackground_data;
            iconQuit            = v02_IconQuit_data;
            iconBack            = v02_IconBack_data;
            iconResume          = v02_IconResume_data;
            shutdownBackground  = v02_MenuBackground_data;
            bigDigits           = v02_BigDigits_data;
            iconLowBattBase     = v02_LowBattBase_data;
            faultMessage        = v02_FaultMessage_data;
            iconCubeRange       = v02_CubeRangePause_data;
            bluetoothPairing    = v02_BluetoothPairing_data;
            menuHeight          = 11;
            menuTextPalette     = 8;
            iconSize            = 8;
            iconSpacing         = 9;
            shutdownHeight      = 11;
            shutdownY1          = 1;
            shutdownY2          = 9;
            break;

    }
}
