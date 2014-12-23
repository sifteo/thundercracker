/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK
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

/*
 * Stateful events are dispatched by setting MenuEvent.type (and
 * MenuEvent.neighbor/MenuEvent.item as appropriate) in a transTo$TATE or
 * state$TATE method and returning immediately.
 *
 * All events are immediately caught by the dispatchEvent method in pollEvent,
 * which copies it into the caller's event structure, and returns control to
 * the caller to handle the event.
 *
 * The default handler for an event, if any, must be explicitly invoked
 * at some point during the event loop, by calling performDefault().
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

inline void Menu::handleNeighborAdd()
{
    MENU_LOG("Default handler: neighborAdd\n");
    // TODO: play a sound
}

inline void Menu::handleNeighborRemove()
{
    MENU_LOG("Default handler: neighborRemove\n");
    // TODO: play a sound
}

inline void Menu::handleItemArrive()
{
    MENU_LOG("Default handler: itemArrive\n");
    // TODO: play a sound
}

inline void Menu::handleItemDepart()
{
    MENU_LOG("Default handler: itemDepart\n");
    // TODO: play a sound
}

inline void Menu::handleItemPress()
{
    MENU_LOG("Default handler: itemPress\n");
    // animate out icon
    changeState(MENU_STATE_FINISH);
}

inline void Menu::handleExit()
{
    MENU_LOG("Default handler: exit\n");
    // nothing
}

inline void Menu::handlePrepaint()
{
    if (hasBeenStarted)
    {
        System::paint();
    }
}

inline void Menu::performDefault()
{
    switch(currentEvent.type) {
        case MENU_NEIGHBOR_ADD:
            handleNeighborAdd();
            break;
        case MENU_NEIGHBOR_REMOVE:
            handleNeighborRemove();
            break;
        case MENU_ITEM_ARRIVE:
            handleItemArrive();
            break;
        case MENU_ITEM_DEPART:
            handleItemDepart();
            break;
        case MENU_ITEM_PRESS:
            handleItemPress();
            break;
        case MENU_EXIT:
            handleExit();
            break;
        case MENU_PREPAINT:
            handlePrepaint();
            break;
        default:
            break;
    }

    // clear event
    clearEvent();
}

inline void Menu::clearEvent()
{
    currentEvent.type = MENU_UNEVENTFUL;
}

inline bool Menu::dispatchEvent(struct MenuEvent *ev)
{
    if (currentEvent.type != MENU_UNEVENTFUL) {
        *ev = currentEvent;
        return true;
    }
    return false;
}

/**
 * @} end addtogroup menu
 */

};  // namespace Sifteo
