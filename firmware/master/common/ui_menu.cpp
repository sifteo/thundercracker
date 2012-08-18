/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "ui_menu.h"
#include "ui_coordinator.h"
#include "vram.h"
#include "cubeslots.h"
#include "cube.h"

// Menu settings
namespace {
    static const unsigned TILE = 8;

    static const unsigned kIconPadding = 3;
    static const unsigned kIconYOffset = 1;

    static const unsigned kNumTilesX = 18;
    static const unsigned kNumVisibleTilesX = 16;
    static const unsigned kEndPadding = 4 * TILE;
    static const unsigned kWindowBorder = 3;

    static const int kAccelHysteresisMin = 8;
    static const int kAccelHysteresisMax = 18;
    static const float kAccelScale = -8.0f;
}

void UIMenu::init(unsigned defaultItem)
{
    if (!uic.isAttached())
        return;

    // Paint entire screen with background tile (We'll see it during the hop animation)
    uint16_t bg = _SYS_TILE77(uic.assets.menuBackground[1]);
    for (unsigned i = 0; i < _SYS_VRAM_BG0_WIDTH * _SYS_VRAM_BG0_WIDTH; ++i)
        VRAM::poke(uic.avb.vbuf, i, bg);

    // Set up default video state
    uic.finish();
    uic.letterboxWindow(TILE * uic.assets.menuHeight);
    VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_ROM);

    // We won't transition to STATIC until we verify that isTouching() == false
    state = S_SETTLING;

    setActiveItem(defaultItem);
    position = itemCenterPosition(defaultItem);

    drawAll();
    uic.setPanX(position);
    uic.setPanY(0);
}

unsigned UIMenu::centerPixelX()
{
    return (kNumVisibleTilesX - uic.assets.iconSize) * (TILE / 2);
}

void UIMenu::animate()
{
    SysTime::Ticks now = SysTime::ticks();
    float dt = (now - lastTime) * (1.0f / SysTime::sTicks(1));
    lastTime = now;

    if (!uic.isAttached())
        return;

    CubeSlot &cube = CubeSlots::instances[uic.avb.cube];

    /*
     * Sample accelerometer, with hysteresis
     */

    _SYSByte4 accel = cube.getAccelState();
    int accelThreshold = state == S_TILTING ? kAccelHysteresisMin : kAccelHysteresisMax;
    bool isTilting = accel.x > accelThreshold || accel.x < -accelThreshold;

    /*
     * Update state machine
     */

    switch (state) {

        case S_SETTLING:
            // Settling: Pull toward and activate the nearest item, if the accelerometer is mostly centered
            if (isTilting) {
                state = S_TILTING;
            } else if (cube.isTouching() && items[activeItem].selectable) {
                // Allow selection even if we're still settling
                beginFinishing();
            } else {
                // Snap to selection (fixed timestep)
                float distance = itemCenterPosition(activeItem) - position;
                updatePosition(distance * 0.1f);
                if (distance < 0.1f && distance > -0.1f && !cube.isTouching()) {
                    state = S_STATIC;
                }
            }
            break;

        case S_TILTING:
            // Tilting: Slide in the direction of the accelerometer (variable timestep)
            updatePosition(accel.x * kAccelScale * dt);
            if (!isTilting) {
                state = S_SETTLING;
            }
            break;

        case S_STATIC:
            // Static: Menu isn't moving, cube wasn't being touched. Has that changed?
            if (isTilting) {
                state = S_TILTING;
            } else if (cube.isTouching() && items[activeItem].selectable) {
                beginFinishing();
            }
            break;

        case S_FINISHING:
            // Finishing: Selected item 'hops' down.
            updateHop();
            break;

        default:
            break;
    }

    /*
     * Update graphics.
     *
     * Note that we finish the previous frame before drawing columns
     * for the next frame, in order to strictly avoid wrap-around artifacts
     * and significantly decrease visible tearing. We can get away with this
     * without causing any noticeable lag, since the rest of our render
     * is so fast.
     */

    uic.finish();

    unsigned newActiveItem = nearestItem();
    if (newActiveItem == activeItem) {
        // Just draw the new portion of the screen that's scrolling in
        drawColumns();
    } else {
        // Active item changed, redraw everything
        setActiveItem(newActiveItem);
        drawAll();
    }

    uic.setPanX(position);
}

void UIMenu::beginFinishing()
{
    /*
     * Entering 'finishing' state. We adjust the window so that the item
     * hops 'behind' the small border at the bottom of the menu, and we
     * force everything to redraw so we can hide the inactive icons.
     */

    activeItem = -1;
    hopFrame = 0;
    state = S_FINISHING;

    // Need to finsh rendering before adjusting the window and panning atomically like this
    uic.finish();
    uic.letterboxWindow(TILE * uic.assets.menuHeight - kWindowBorder * 2);
    updateHop();
}

void UIMenu::setActiveItem(unsigned n)
{
    activeItem = n;
    labelWidth = n < numItems ? strlen(items[n].label) : 0;
}

int UIMenu::itemCenterPosition(unsigned n)
{
    return uic.assets.iconSpacing * TILE * n - centerPixelX();
}

unsigned UIMenu::nearestItem()
{
    const int pixelSpacing = uic.assets.iconSpacing * TILE;
    int item = (position + centerPixelX() + pixelSpacing/2) / pixelSpacing;
    return MAX(0, MIN(numItems - 1, item));
}

void UIMenu::updatePosition(float velocity)
{
    if (velocity >  float(TILE)) velocity = float(TILE);
    if (velocity < -float(TILE)) velocity = -float(TILE);

    position += velocity;

    int center = centerPixelX();
    int leftLimit = -kEndPadding - center;
    int rightLimit = uic.assets.iconSpacing * TILE * (numItems - 1) + kEndPadding - center;
    int pixelPosition = position;
    if (pixelPosition < leftLimit)  position = leftLimit;
    if (pixelPosition > rightLimit) position = rightLimit;
}

void UIMenu::updateHop()
{
    // Update the vertical 'hop' animation (Y panning)

    static const int8_t hop[] = {
        // [int(i*2.5 - i*i*0.3 + 0.5) for i in range(1, 24)]
        2, 4, 5, 5, 5, 4, 3, 1, -1, -4, -8, -12, -17, -23,
        -29, -36, -43, -51, -60, -69, -79, -89, -100
    };

    ASSERT(hopFrame < arraysize(hop));
    uic.setPanY(kWindowBorder + hop[hopFrame]);

    if (hopFrame == 17) {
        // Partway through, clear the bottom half of the icon before it wraps around

        unsigned begin = kNumTilesX * (kIconYOffset + uic.assets.iconSize/2);
        unsigned end = kNumTilesX * uic.assets.menuHeight;
        uint16_t bg = _SYS_TILE77(uic.assets.menuBackground[1]);

        for (unsigned i = begin; i != end; ++i)
            VRAM::poke(uic.avb.vbuf, i, bg);
    }

    if (++hopFrame == arraysize(hop))
        state = S_DONE;
}

void UIMenu::drawAll()
{
    int tilePosition = int(position) / int(TILE);
    prevTilePosition = tilePosition;
    for (int i = -1; i < int(kNumVisibleTilesX) + 1; ++i) {
        drawColumn(tilePosition + i);
    }
}

void UIMenu::drawColumns()
{
    // Fill in tiles ahead of the direction of scrolling
    int tilePosition = int(position) / int(TILE);
    while (tilePosition > prevTilePosition) {
        drawColumn(++prevTilePosition + kNumVisibleTilesX);
    }
    while (tilePosition < prevTilePosition) {
        drawColumn(--prevTilePosition - 1);
    }
}

void UIMenu::drawColumn(int x)
{
    // 'x' is a column index, in tiles, from the origin of our virtual coordinate
    // system. It is mapped modulo-18 onto BG0.

    unsigned iconSize = uic.assets.iconSize;
    unsigned iconSpacing = uic.assets.iconSpacing;
    unsigned textPalette = unsigned(uic.assets.menuTextPalette) << 10;
    unsigned addr = umod(x, kNumTilesX);

    for (int y = 0; y < uic.assets.menuHeight; ++y) {
        // Don't draw borders when finishing. Otherwise, draw the whole background.
        uint16_t tile = uic.assets.menuBackground[state == S_FINISHING ? 1 : y];

        unsigned iconIndex = x / iconSpacing;
        unsigned iconX = x % iconSpacing;
        unsigned iconY = y - kIconYOffset;

        if (iconX < iconSize && iconY < iconSize && iconIndex < numItems) {
            bool active = iconIndex == activeItem;

            if (state != S_FINISHING || active) {
                // Inactive icon is below active icon
                unsigned frameY = iconY + (active ? 0 : iconSize);
                tile = uic.assets.images[items[iconIndex].index][iconX + frameY * iconSize];
            }
        }

        if (y == (uic.assets.menuHeight - 2) && labelWidth) {
            // Drawing the label text

            unsigned labelX = x - iconSpacing * activeItem + labelWidth/2 - iconSize/2;
            if (labelX < labelWidth) {
                // Note the uint8_t cast. This is important to avoid sign-extending non-ASCII characters!
                tile = uint8_t(items[activeItem].label[labelX] - ' ') ^ textPalette;
            }
        }

        VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(tile));
        addr += kNumTilesX;
    }
}
