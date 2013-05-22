/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "ui_bluetoothpairing.h"
#include "ui_coordinator.h"
#include "vram.h"


void UIBluetoothPairing::init()
{
    if (!uic.isAttached())
        return;

    uic.finish();
    uic.letterboxWindow(128);
    uic.setMode();
    uic.setPanX(-4);
    uic.setPanY(-2);

    drawBackground();

    drawDigit(uic.xy(2 , 3), code[0]);
    drawDigit(uic.xy(6,  3), code[1]);
    drawDigit(uic.xy(10, 3), code[2]);
    drawDigit(uic.xy(2,  9), code[3]);
    drawDigit(uic.xy(6,  9), code[4]);
    drawDigit(uic.xy(10, 9), code[5]);

    uic.paint();
    uic.finish();
}

void UIBluetoothPairing::drawBackground()
{
    /*
     * To save space, we store the background collapsed into an 18x5 tile image.
     * The fourth row in the image needs to be repeated 12 times.
     */

    unsigned addr = 0;
    const uint16_t *src = uic.assets.bluetoothPairing;

    uic.drawTiles(addr, src, 18 * 3);
    addr += 18 * 3;
    src += 18 * 3;

    for (unsigned i = 12; i; --i) {
        uic.drawTiles(addr, src, 18);
        addr += 18;
    }
    src += 18;

    uic.drawTiles(addr, src, 18);
    addr += 18;

    // Fill the last two lines with copies of the first tile (background)
    for (unsigned i = 18 * 2; i; --i) {
        uic.drawTiles(addr, uic.assets.bluetoothPairing, 1);
        addr++;
    }
}

void UIBluetoothPairing::drawDigit(unsigned addr, char digit)
{
    /*
     * We want 3x5 tile digits. Our 4x5 tile digits will work if
     * we skip the second column of tiles. Do this transform,
     * and also recolor our tiles to match the background.
     */

    unsigned number = digit - '0';
    if (number > 9)
        return;

    const uint16_t palette = uic.assets.bluetoothPairing[ uic.xy(2,3) ];
    const uint16_t *frame = uic.assets.bigDigits + number * (4*5);

    for (unsigned y = 0; y < 5; y++) {
        for (unsigned x = 0; x < 4; x++) {
            if (x != 1) {
                unsigned tile = palette ^ frame[x + y*4];
                VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(tile));
                addr++;
            }
        }
        addr += 18 - 3;
    }
}
