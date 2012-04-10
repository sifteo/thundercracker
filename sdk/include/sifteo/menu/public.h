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
    shouldPaintSync = false;
    const float kAccelScalingFactor = -0.25f;
    xaccel = kAccelScalingFactor * pCube->virtualAccel().x;
    yaccel = kAccelScalingFactor * pCube->virtualAccel().y;

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
     * the caller doesn't know whether to paintSync or paint and shouldn't be
     * painting while the menu owns the context.
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

inline void Menu::paintSync()
{
    // this is only relevant when caller is handling a PREPAINT event.
    ASSERT(currentEvent.type == MENU_PREPAINT);
    shouldPaintSync = true;
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
