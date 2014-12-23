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

#ifndef _UI_MENU_H
#define _UI_MENU_H

#include <sifteo/abi.h>
#include "systime.h"

class UICoordinator;


/**
 * A simplified tilt-flow menu, for use within the firmware.
 *
 * This has been designed to be consistent with the SDK menu
 * where applicable, but a lot of things are different: We're
 * operating in BG0_ROM with no flash resources, and we have only
 * uncompressed asset images. Our labels are ASCII text. And finally,
 * the screen layout is different because our icons are small and
 * we're operating in a narrow window.
 */

class UIMenu {
public:
    struct Item {
        const char *label;
        uint8_t index;
        bool selectable;
    };

    UIMenu(UICoordinator &uic, const Item *items, unsigned numItems)
        : uic(uic), items(items), numItems(numItems) {}

    void init(unsigned defaultItem);
    void animate();

    ALWAYS_INLINE bool isDone() const {
        return state == S_DONE;
    }

    ALWAYS_INLINE unsigned getChosenItem() const {
        ASSERT(isDone());
        ASSERT(activeItem < numItems);
        return activeItem;
    }

private:
    // Static parameters
    UICoordinator &uic;
    const Item *items;

    enum State {
        S_SETTLING,
        S_TILTING,
        S_STATIC,
        S_FINISHING,
        S_DONE,
    };

    SysTime::Ticks lastTime;    // Timestamp for the last call to animate()
    float position;             // Panning offset in pixels
    uint16_t activeItem;        // If < numItems, this menu item is selected
    uint16_t labelWidth;        // Size of label associated with 
    int16_t prevTilePosition;   // X tile offset of last draw
    uint8_t state;              // State enum
    uint8_t hopFrame;           // For Hop animation in S_FINISHING
    uint8_t numItems;           // Total number of items in menu

    void updatePosition(float velocity);
    void updateHop();
    void beginFinishing();
    void drawAll();
    void drawColumns();
    void drawColumn(int x);
    void setActiveItem(unsigned n);
    int itemCenterPosition(unsigned n);
    unsigned centerPixelX();
    unsigned nearestItem();
};


#endif
