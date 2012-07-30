/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
