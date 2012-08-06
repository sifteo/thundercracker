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

inline Menu::Menu(VideoBuffer &vid, const MenuAssets *aAssets, MenuItem *aItems)
{
    init(vid, aAssets, aItems);
}

inline void Menu::init(VideoBuffer &vid, const MenuAssets *aAssets, MenuItem *aItems)
{
    this->vid = &vid;
    hasBeenStarted = false;

    currentEvent.type = MENU_UNEVENTFUL;
    items = aItems;
    assets = aAssets;
    changeState(MENU_STATE_START);
    
    // initialize instance constants
    kHeaderHeight = 0;
    kFooterHeight = 0;
    kIconTileWidth = 0;

    // calculate the number of items
    uint8_t i = 0;
    while (items[i].icon != NULL) {
        if (kIconTileWidth == 0) {
            kIconTileWidth = items[i].icon->tileWidth();
            kIconTileHeight = items[i].icon->tileHeight();
            kEndCapPadding = (kNumVisibleTilesX - kIconTileWidth) * (TILE / 2.f);
            ASSERT((kItemPixelWidth() - kIconPixelWidth()) % TILE == 0);
        } else {
            ASSERT(items[i].icon->tileWidth() == kIconTileWidth);
            ASSERT(items[i].icon->tileHeight() == kIconTileHeight);
        }

        i++;
        if (items[i].label != NULL) {
            ASSERT(items[i].label->tileWidth() == kNumVisibleTilesX);
            if (kHeaderHeight == 0) {
                kHeaderHeight = items[i].label->tileHeight();
            } else {
                ASSERT(items[i].label->tileHeight() == kHeaderHeight);
            }
            /* XXX: if there are any labels, a header is required for now.
             * supporting labels and no header would require toggling bg0
             * tiles fairly often and that's state I don't want to deal with
             * for the time being.
             * workaround: header can be an appropriately-sized, entirely
             * transparent PNG.
             */
            ASSERT(assets->header != NULL);
        }
    }
    numItems = i;

    // calculate the number of tips
    i = 0;
    while (assets->tips[i] != NULL) {
        ASSERT(assets->tips[i]->tileWidth() == kNumVisibleTilesX);
        if (kFooterHeight == 0) {
            kFooterHeight = assets->tips[i]->tileHeight();
        } else {
            ASSERT(assets->tips[i]->tileHeight() == kFooterHeight);
        }
        i++;
    }
    numTips = i;

    // sanity check the rest of the assets
    ASSERT(assets->background);
    ASSERT(assets->background->tileWidth() == 1 && assets->background->tileHeight() == 1);
    if (assets->footer) {
        ASSERT(assets->footer->tileWidth() == kNumVisibleTilesX);
        if (kFooterHeight == 0) {
            kFooterHeight = assets->footer->tileHeight();
        } else {
            ASSERT(assets->footer->tileHeight() == kFooterHeight);
        }
    }

    if (assets->header) {
        ASSERT(assets->header->tileWidth() == kNumVisibleTilesX);
        if (kHeaderHeight == 0) {
            kHeaderHeight = assets->header->tileHeight();
        } else {
            ASSERT(assets->header->tileHeight() == kHeaderHeight);
        }
    }

    prev_ut = 0;
    startingItem = 0;

    position = 0.0f;

    setIconYOffset(kDefaultIconYOffset);
    setPeekTiles(kDefaultPeekTiles);
}

inline VideoBuffer * Menu::videoBuffer() const
{
    return vid;
}

inline CubeID Menu::cube() const
{
    return vid ? vid->cube() : CubeID();
}

inline bool Menu::pollEvent(struct MenuEvent *ev)
{
    // Events not handled at this point are discarded
    ASSERT(currentEvent.type != MENU_PREPAINT);
    clearEvent();
    
    /* state changes can happen in the default event handler which may dispatch
     * events (like MENU_STATE_STATIC -> MENU_STATE_FINISH dispatches a
     * MENU_PREPAINT).
     */
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // keep track of time so if our framerate changes, apparent speed persists
    frameclock.next();

    // neighbor changes?
    if (currentState != MENU_STATE_START) {
        detectNeighbors();
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // update commonly-used data
    const float kAccelScalingFactor = -0.25f;
    accel = kAccelScalingFactor * vid->virtualAccel().xy();

    // state changes
    switch (currentState) {
        case MENU_STATE_START:
            transFromStart();
            break;
        case MENU_STATE_STATIC:
            transFromStatic();
            break;
        case MENU_STATE_TILTING:
            transFromTilting();
            break;
        case MENU_STATE_INERTIA:
            transFromInertia();
            break;
        case MENU_STATE_FINISH:
            transFromFinish();
            break;
        case MENU_STATE_HOP_UP:
            transFromHopUp();
            break;
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // run loop
    switch (currentState) {
        case MENU_STATE_START:
            stateStart();
            break;
        case MENU_STATE_STATIC:
            stateStatic();
            break;
        case MENU_STATE_TILTING:
            stateTilting();
            break;
        case MENU_STATE_INERTIA:
            stateInertia();
            break;
        case MENU_STATE_FINISH:
            stateFinish();
            break;
        case MENU_STATE_HOP_UP:
            stateHopUp();
            break;
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // no special events, paint a frame.
    drawFooter();
    currentEvent.type = MENU_PREPAINT;
    dispatchEvent(ev);
    return true;
}

inline void Menu::reset()
{
    changeState(MENU_STATE_START);
}

inline void Menu::replaceIcon(uint8_t item, const AssetImage *icon, const AssetImage *label)
{
    ASSERT(item < numItems);

    items[item].icon = icon;
    for (int i = prev_ut; i < prev_ut + kNumTilesX; i++)
        if (itemVisibleAtCol(item, i))
            drawColumn(i);

    if (label) {
        uint8_t currentItem = computeSelected();
        items[item].label = label;
        
        if (kHeaderHeight && currentState == MENU_STATE_STATIC &&
            currentItem == item)
        {
            const AssetImage& label = items[currentItem].label
                                    ? *items[currentItem].label
                                    : *assets->header;
            vid->bg1.image(vec(0,0), label);
        }
    }
}

inline bool Menu::itemVisible(uint8_t item)
{
    ASSERT(item >= 0 && item < numItems);

    for (int i = MAX(0, prev_ut); i < prev_ut + kNumTilesX; i++) {
        if (itemVisibleAtCol(item, i)) return true;
    }
    return false;
}

inline void Menu::setIconYOffset(uint8_t px)
{
    ASSERT(px >= 0 || px < kNumTilesX * 8);
    kIconYOffset = -px;
    if (hasBeenStarted)
        updateBG0();
}

inline void Menu::setPeekTiles(uint8_t numTiles)
{
    ASSERT(numTiles >= 1 || numTiles * 2 < kNumTilesX);
    kPeekTiles = numTiles;
    if (hasBeenStarted)
        updateBG0();
}

/**
 * Set the menu anchor.
 *
 * The anchor item is the active item in the menu when the menu starts. If the
 * menu has already started, calling this method affects the future invocations
 * of the same menu since running the event pump after an item is pressed
 * restarts the menu.
 */
inline void Menu::anchor(uint8_t item, bool hopUp)
{
    ASSERT(item < numItems);
    startingItem = item;
    
    if (hopUp) {
        position = stoppingPositionFor(startingItem);
        prev_ut = computeCurrentTile() + kNumTilesX;
        updateBG0();
        
        changeState(MENU_STATE_HOP_UP);
    }
}

inline MenuState Menu::getState()
{
    return currentState;
}

/**
 * @} end addtogroup menu
 */

};  // namespace Sifteo

