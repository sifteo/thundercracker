/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MENU_PUBLIC_H
#define _SIFTEO_MENU_PUBLIC_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/menu/types.h>

namespace Sifteo {


inline Menu::Menu(VideoBuffer &vid, struct MenuAssets *aAssets,
    struct MenuItem *aItems)
    : vid(vid)
{
    currentEvent.type = MENU_UNEVENTFUL;
    changeState(MENU_STATE_START);
    items = aItems;
    assets = aAssets;

    // initialize instance constants
    kHeaderHeight = 0;
    kFooterHeight = 0;
    kIconTileWidth = 0;

    // calculate the number of items
    uint8_t i = 0;
    while(items[i].icon != NULL) {
        if (kIconTileWidth == 0) {
            kIconTileWidth = items[i].icon->tileWidth();
            kIconTileHeight = items[i].icon->tileHeight();
            kEndCapPadding = (kNumVisibleTilesX - kIconTileWidth) * (TILE / 2.f);
            ASSERT((kItemPixelWidth() - kIconPixelWidth()) % TILE == 0);
            // icons should leave at least one tile on both sides for the next/prev items
            ASSERT(kIconTileWidth <= kNumVisibleTilesX - (kPeekTiles * 2));
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
    while(assets->tips[i] != NULL) {
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

    setIconYOffset(kDefaultIconYOffset);
}

inline bool Menu::pollEvent(struct MenuEvent *ev)
{
    // handle/clear pending events
    if (currentEvent.type != MENU_UNEVENTFUL) {
        performDefault();
    }
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
    accel = kAccelScalingFactor * vid.virtualAccel().xy();

    // state changes
    switch(currentState) {
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
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // run loop
    switch(currentState) {
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

inline void Menu::preventDefault()
{
    /* paints shouldn't be prevented because:
     * the caller shouldn't be painting while the menu owns the context.
     */
    ASSERT(currentEvent.type != MENU_PREPAINT);
    /* exit shouldn't be prevented because:
     * the default handler is responsible for resetting the menu if the event
     * pump is restarted.
     */
    ASSERT(currentEvent.type != MENU_EXIT);

    clearEvent();
}

inline void Menu::reset()
{
    changeState(MENU_STATE_START);
}

inline void Menu::replaceIcon(uint8_t item, const AssetImage *icon)
{
    ASSERT(item < numItems);
    items[item].icon = icon;

    for(int i = prev_ut; i < prev_ut + kNumTilesX; i++)
        if (itemVisibleAtCol(item, i))
            drawColumn(i);
}

inline bool Menu::itemVisible(uint8_t item)
{
    ASSERT(item >= 0 && item < numItems);

    for(int i = MAX(0, prev_ut); i < prev_ut + kNumTilesX; i++) {
        if (itemVisibleAtCol(item, i)) return true;
    }
    return false;
}

inline void Menu::setIconYOffset(uint8_t px)
{
    ASSERT(px >= 0 || px < kNumTilesX * 8);
    kIconYOffset = -px;
    updateBG0();
}


};  // namespace Sifteo

#endif
