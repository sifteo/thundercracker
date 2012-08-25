/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
