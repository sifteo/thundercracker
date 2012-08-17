/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/menu/types.h>

namespace Sifteo {

/**
 * @addtogroup menu
 * @{
 */

inline void Menu::detectNeighbors()
{
    Neighborhood nbr(vid->cube());

    for (Side i = Side(0); i < NUM_SIDES; i++) {
        MenuNeighbor n;

        if (nbr.hasCubeAt(i)) {
            CubeID c(nbr.cubeAt(i));
            n.neighborSide = Neighborhood(c).sideOf(vid->cube());
            n.neighbor = c;
            n.masterSide = i;
        } else {
            n.neighborSide = NO_SIDE;
            n.neighbor = CubeID::UNDEFINED;
            n.masterSide = NO_SIDE;
        }

        if (n != neighbors[i]) {
            // Neighbor was set but is now different/gone
            if (neighbors[i].neighbor != CubeID::UNDEFINED) {
                // Generate a lost neighbor event, with the ghost of the lost cube.
                currentEvent.type = MENU_NEIGHBOR_REMOVE;
                currentEvent.neighbor = neighbors[i];

                /* Act as if the neighbor was lost, even if it wasn't. We'll
                 * synthesize another event on the next run of the event loop
                 * if the neighbor was replaced by another cube in less than
                 * one polling cycle.
                 */
                neighbors[i].neighborSide = NO_SIDE;
                neighbors[i].neighbor = CubeID::UNDEFINED;
                neighbors[i].masterSide = NO_SIDE;
            } else {
                neighbors[i] = n;
                currentEvent.type = MENU_NEIGHBOR_ADD;
                currentEvent.neighbor = n;
            }

            /* We don't have a method of queueing events, so return as soon as
             * a change is discovered. If there are other changes, the next run
             * of the event loop will discover them.
             */
            return;
        }
    }
}

inline uint8_t Menu::computeSelected()
{
    int s = (position + (kItemPixelWidth() / 2)) / kItemPixelWidth();
    return clamp(s, 0, numItems - 1);
}

inline void Menu::checkForPress()
{
    bool touch = vid->cube().isTouching();

    if (touch && !prevTouch) {
        currentEvent.type = MENU_ITEM_PRESS;
        currentEvent.item = computeSelected();
    }
    prevTouch = touch;
}

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
inline void Menu::drawColumn(int x)
{
    // x is the column in "global" space
    Int2 topLeft = { umod(x, kNumTilesX), 0 };
    Int2 size = { 1, kIconTileHeight };

    // icon or blank column?
    if (itemAtCol(x) < numItems) {
        // drawing an icon column
        const AssetImage &img = *items[itemAtCol(x)].icon;
        Int2 srcXY = { ((x % kItemTileWidth()) % img.tileWidth()), 0 };
        vid->bg0.image(topLeft, size, img, srcXY);
    } else {
        if (assets->overflowIcon && umod(x,kIconTileWidth) < kItemTileWidth()) {
            // drawing the overflow icon
            x = umod(x, kItemTileWidth());
            const AssetImage &img = *assets->overflowIcon;
            Int2 srcXY = { ((x % kItemTileWidth()) % img.tileWidth()), 0 };
            vid->bg0.image(topLeft, size, img, srcXY);
        } else {
            // drawing a blank column
            vid->bg0.fill(topLeft, size, *assets->background);
        }
    }
}

inline void Menu::drawFooter(bool force)
{
    if (numTips == 0) return;

    const AssetImage& footer = *assets->tips[currentTip];
    const float kSecondsPerTip = 4.f;

    if (SystemTime::now() - prevTipTime > kSecondsPerTip || force) {
        prevTipTime = SystemTime::now();

        if (numTips > 0) {
            currentTip = (currentTip+1) % numTips;
        }

        Int2 topLeft = { 0, kNumVisibleTilesY - footer.tileHeight() };
        vid->bg1.image(topLeft, footer);
    }
}

inline int Menu::stoppingPositionFor(int selected)
{
    return kItemPixelWidth() * selected;
}

inline float Menu::velocityMultiplier()
{
    return abs(accel.x) > kAccelThresholdStep ? (1.f * kMaxSpeedMultiplier) : 1.f;
}

inline float Menu::maxVelocity()
{
    const float kMaxNormalSpeed = 40.f;
    return kMaxNormalSpeed *
        // x-axis linear limit
        (abs(accel.x) / kOneG()) *
        // y-axis multiplier
        velocityMultiplier();
}

inline float Menu::lerp(float min, float max, float u)
{
    return min + u * (max - min);
}

inline void Menu::updateBG0()
{
    int ut = computeCurrentTile();

    while(prev_ut < ut) {
        drawColumn(++prev_ut + kNumVisibleTilesX);
    }
    while(prev_ut > ut) {
        drawColumn(--prev_ut);
    }

    {
        Int2 vec = {(position - kEndCapPadding), kIconYOffset};
        vid->bg0.setPanning(vec);
    }
}

inline bool Menu::itemVisibleAtCol(uint8_t item, int column)
{
    ASSERT(item >= 0 && item < numItems);
    if (column < 0) return false;

    return itemAtCol(column) == item;
}

inline uint8_t Menu::itemAtCol(int column)
{
    if (column < 0) return numItems;

    if (column % kItemTileWidth() < kIconTileWidth) {
        return column < 0 ? numItems : column / kItemTileWidth();
    }
    return numItems;
}

inline int Menu::computeCurrentTile()
{
    /* these are necessary if the icon widths are an odd number of tiles,
     * because the position is no longer aligned with the tile domain.
     */

    const int kPixelsPerTile = TILE;
    const int kPositionAlignment = kEndCapPadding % kPixelsPerTile == 0
        ? 0 : kPixelsPerTile - kEndCapPadding % kPixelsPerTile;
    const int kTilePadding = kEndCapPadding / kPixelsPerTile;

    int ui = position - kPositionAlignment;
    int ut = (ui < 0 ? ui - kPixelsPerTile : ui) / kPixelsPerTile; // special case because int rounds up when < 0
    ut -= kTilePadding;

    return ut;
}

/**
 * @} end addtogroup menu
 */

};  // namespace Sifteo
