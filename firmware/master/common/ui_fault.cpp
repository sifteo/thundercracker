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
#include "ui_fault.h"
#include "vram.h"
#include "homebutton.h"


void UIFault::init()
{
    if (!uic.isAttached())
        return;

    uic.finish();

    /*
     * Draw screen. Most of this is static, but we have
     * a reference number, in decimal, added to the tile
     * data at the right spot.
     */

    const unsigned refOnesPlace = 14 + 14*18;
    const unsigned refNumDigits = 4;
    unsigned divisor = 1000;

    const uint16_t *src = uic.assets.faultMessage;
    unsigned dest = 0;

    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 16; ++x) {

            uint16_t tile = *(src++);
            unsigned refDigit = refOnesPlace - dest;

            if (refDigit < refNumDigits) {
                tile += (reference / divisor) % 10;
                divisor /= 10;
            }

            VRAM::poke(uic.avb.vbuf, dest, _SYS_TILE77(tile));
            dest++;
        }
        dest += 2;
    }

    // Set up default video state
    VRAM::poke(uic.avb.vbuf, offsetof(_SYSVideoRAM, first_line)/2, 0x8000);
    VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_ROM);
    uic.setPanX(0);
    uic.setPanY(0);
}

void UIFault::mainLoop()
{
    // Home button press only counts if we've seen it released first
    bool haveSeenRelease = HomeButton::isReleased();

    do {
        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            init();

        uic.paint();

        if (HomeButton::isReleased())
            haveSeenRelease = true;

    } while (!uic.isTouching() && !(haveSeenRelease && HomeButton::isPressed()));

    // Dismiss UI
    uic.restoreCubes(uic.uiConnected);
}
