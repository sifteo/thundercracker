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

#ifndef _UI_ASSETS_H
#define _UI_ASSETS_H

#include "macros.h"
#include <sifteo/abi.h>


/**
 * Information about versioned assets used by system UI.
 *
 * We may have to use different system menu assets depending on the
 * version of the cube's Tile ROM. This object stores all of the
 * version-specific details we need for the individual UIs.
 */

struct UIAssets {

    void init(unsigned version);

    union {
        const uint16_t *images[1];
        struct {
            const uint16_t *logoWhiteOnBlue;
            const uint16_t *menuBackground;
            const uint16_t *iconQuit;
            const uint16_t *iconBack;
            const uint16_t *iconResume;
            const uint16_t *iconLowBattBase;
            const uint16_t *shutdownBackground;
            const uint16_t *bigDigits;
            const uint16_t *faultMessage;
            const uint16_t *iconCubeRange;
            const uint16_t *bluetoothPairing;
        };
    };

    int8_t menuHeight;
    uint8_t menuTextPalette;
    int8_t iconSize;
    int8_t iconSpacing;
    int8_t shutdownHeight;
    int8_t shutdownY1;
    int8_t shutdownY2;
};

// Calculate an index into images[]
#define UI_IMAGE_INDEX(_name)    ((offsetof(UIAssets, _name) - offsetof(UIAssets, images)) / sizeof(uint16_t*))

#endif
