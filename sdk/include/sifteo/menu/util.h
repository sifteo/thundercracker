/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MENU_UTIL_H
#define _SIFTEO_MENU_UTIL_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/menu/types.h>

namespace Sifteo {


inline void Menu::detectNeighbors()
{
    for(_SYSSideID i = 0; i < NUM_SIDES; i++) {
        MenuNeighbor n;
        if (pCube->hasPhysicalNeighborAt(i)) {
            Cube c(pCube->physicalNeighborAt(i));
            n.neighborSide = c.physicalSideOf(pCube->id());
            n.neighbor = c.id();
            n.masterSide = i;
        } else {
            n.neighborSide = SIDE_UNDEFINED;
            n.neighbor = CUBE_ID_UNDEFINED;
            n.masterSide = SIDE_UNDEFINED;
        }

        if (n != neighbors[i]) {
            // Neighbor was set but is now different/gone
            if (neighbors[i].neighbor != CUBE_ID_UNDEFINED) {
                // Generate a lost neighbor event, with the ghost of the lost cube.
                currentEvent.type = MENU_NEIGHBOR_REMOVE;
                currentEvent.neighbor = neighbors[i];

                /* Act as if the neighbor was lost, even if it wasn't. We'll
                 * synthesize another event on the next run of the event loop
                 * if the neighbor was replaced by another cube in less than
                 * one polling cycle.
                 */
                neighbors[i].neighborSide = SIDE_UNDEFINED;
                neighbors[i].neighbor = CUBE_ID_UNDEFINED;
                neighbors[i].masterSide = SIDE_UNDEFINED;
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
    int s = (position + (kItemPixelWidth / 2)) / kItemPixelWidth;
    return clamp(s, 0, numItems - 1);
}


// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
inline void Menu::drawColumn(int x)
{
    // x is the column in "global" space
    uint16_t addr = unsignedMod(x, kNumTilesX);

    // icon or blank column?
    if (itemAtCol(x) < numItems) {
        // drawing an icon column
        const AssetImage* pImg = items[itemAtCol(x)].icon;
        const uint16_t *src = pImg->tiles + ((x % kItemTileWidth) % pImg->width);

        for(int row=0; row<kIconTileHeight; ++row) {
            _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
            addr += kNumTilesX;
            src += pImg->width;

        }
    } else {
        // drawing a blank column
        for(int row=0; row<kIconTileHeight; ++row) {
            Int2 vec = { addr, row };
            canvas.BG0_drawAsset(vec, *assets->background);
        }
    }
}

inline unsigned Menu::unsignedMod(int x, unsigned y)
{
    const int z = x % (int)y;
    return z < 0 ? z+y : z;
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
        
        _SYS_vbuf_writei(
            &pCube->vbuf.sys, 
            offsetof(_SYSVideoRAM, bg1_tiles) / 2 + kFooterBG1Offset,
            footer.tiles, 
            0, 
            footer.width * footer.height
        );
    }
}

inline int Menu::stoppingPositionFor(int selected)
{
    return kItemPixelWidth * selected;
}

inline float Menu::velocityMultiplier()
{
    return yaccel > kAccelThresholdOff ? (1.f + (yaccel * kMaxSpeedMultiplier / kOneG)) : 1.f;
}

inline float Menu::maxVelocity()
{
    const float kMaxNormalSpeed = 40.f;
    return kMaxNormalSpeed *
           // x-axis linear limit
           (abs(xaccel) / kOneG) *
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
        canvas.BG0_setPanning(vec);
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

    if (column % kItemTileWidth < kIconTileWidth) {
        return column < 0 ? numItems : column / kItemTileWidth;
    }
    return numItems;
}

inline int Menu::computeCurrentTile()
{
    /* these are necessary if the icon widths are an odd number of tiles,
     * because the position is no longer aligned with the tile domain.
     */
    const int kPositionAlignment = kEndCapPadding % TILE == 0 ? 0 : TILE - kEndCapPadding % TILE;
    const int kTilePadding = kEndCapPadding / TILE;

    int ui = position - kPositionAlignment;
    int ut = (ui < 0 ? ui - TILE : ui) / TILE; // special case because int rounds up when < 0
    ut -= kTilePadding;

    return ut;
}


};  // namespace Sifteo

#endif
