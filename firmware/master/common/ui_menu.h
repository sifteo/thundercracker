/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
        const uint16_t *icon;
        const char *label;
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

    float position;             // Panning offset in pixels
    uint16_t activeItem;        // If < numItems, this menu item is selected
    uint16_t labelWidth;        // Size of label associated with 
    int16_t prevTilePosition;   // X tile offset of last draw
    uint8_t state;              // State enum
    uint8_t hopFrame;           // For Hop animation in S_FINISHING
    uint8_t numItems;           // Total number of items in menu

    void updatePosition(float velocity);
    void updateHop();
    void drawAll();
    void drawColumns();
    void drawColumn(int x);
    void setActiveItem(unsigned n);
    int itemCenterPosition(unsigned n);
    unsigned nearestItem();
};


#endif
