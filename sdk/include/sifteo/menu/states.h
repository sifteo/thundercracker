/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * States are represented by three functions per state:
 * transTo$TATE() - called whenever transitioning into the state.
 * state$TATE() - the state itself, called every loop of the event pump.
 * transFrom$TATE() - called before every loop of the event pump, responsible
 *                    for transitioning to other states.
 *
 * States are identified by MENU_STATE_$TATE values in enum MenuState, and are
 * mapped to their respective functions in changeState and pollEvent.
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

inline void Menu::changeState(MenuState newstate)
{
    stateFinished = false;
    currentState = newstate;

    MENU_LOG("STATE: -> ");
    switch(currentState) {
        case MENU_STATE_START:
            MENU_LOG("start\n");
            transToStart();
            break;
        case MENU_STATE_STATIC:
            MENU_LOG("static\n");
            transToStatic();
            break;
        case MENU_STATE_TILTING:
            MENU_LOG("tilting\n");
            transToTilting();
            break;
        case MENU_STATE_INERTIA:
            MENU_LOG("inertia\n");
            transToInertia();
            break;
        case MENU_STATE_FINISH:
            MENU_LOG("finish\n");
            transToFinish();
            break;
        case MENU_STATE_HOP_UP:
            MENU_LOG("hop up\n");
            transToHopUp();
            break;
    }
}

/**
 * Start state: MENU_STATE_START
 *
 * This state is responsible for initialization of a potentially dirty menu
 * object, including setting up the video modes and animating the menu in.
 * Transitions:
 * -> Static when initialization is complete and menu becomes interactive.
 * Events:
 * none.
 */
inline void Menu::transToStart()
{
    // Do nothing
}

inline void Menu::stateStart()
{
    // initialize video state

    vid->initMode(BG0_SPR_BG1);
    vid->bg0.erase(*assets->background);

    // Allocate tiles for the static upper label, and draw it.
    if (kHeaderHeight) {
        const AssetImage& label = items[startingItem].label ? *items[startingItem].label : *assets->header;
        vid->bg1.fillMask(vec(0,0), label.tileSize());
        vid->bg1.image(vec(0,0), label);
    }

    // Allocate tiles for the footer, and draw it.
    if (kFooterHeight) {
        const AssetImage& footer = assets->tips[0] ? *assets->tips[0] : *assets->footer;
        Int2 topLeft = { 0, kNumVisibleTilesY - footer.tileHeight() };
        vid->bg1.fillMask(topLeft, footer.tileSize());
        vid->bg1.image(topLeft, footer);
    }

    currentTip = 0;
    prevTipTime = SystemTime::now();
    drawFooter(true);

    // if/when we start animating the menu into existence, set this once the work is complete
    stateFinished = true;
}

inline void Menu::transFromStart()
{
    if (stateFinished) {
        hasBeenStarted = true;
        
        position = stoppingPositionFor(startingItem);
        prev_ut = computeCurrentTile() + kNumTilesX;
        updateBG0();

        for(int i = 0; i < NUM_SIDES; i++) {
            neighbors[i].neighborSide = NO_SIDE;
            neighbors[i].neighbor = CubeID::UNDEFINED;
            neighbors[i].masterSide = NO_SIDE;
        }

        changeState(MENU_STATE_STATIC);
    }
}

/**
 * Static state: MENU_STATE_STATIC
 *
 * The resting state of the menu, the MENU_ITEM_PRESS event is fired from this
 * state on a new touch event.
 * Transitions:
 * -> Tilting when x accelerometer exceeds kAccelThresholdOn.
 * Events:
 * MENU_ITEM_ARRIVE fired when entering this state.
 * MENU_ITEM_PRESS fired when cube is touched in this state.
 * MENU_ITEM_DEPART fired when leaving this state due to tilt.
 */
inline void Menu::transToStatic()
{
    velocity = 0;
    prevTouch = vid->cube().isTouching();

    currentEvent.type = MENU_ITEM_ARRIVE;
    currentEvent.item = computeSelected();

    // show the title of the item
    if (kHeaderHeight) {
        const AssetImage& label = items[currentEvent.item].label ? *items[currentEvent.item].label : *assets->header;
        vid->bg1.image(vec(0,0), label);
    }
}

inline void Menu::stateStatic()
{
    checkForPress();
}

inline void Menu::transFromStatic()
{
    if (abs(accel.x) < kAccelThresholdOn)
        return;

    /*
     * if we're tilting up against either the beginning or the end of the menu,
     * don't generate a new event - there are no more items to navigate to.
     */
    int8_t direction = accel.x > 0 ? 1 : -1;
    if ((currentEvent.item == 0 && direction < 0) ||
        (currentEvent.item == numItems - 1 && direction > 0))
        return;

    changeState(MENU_STATE_TILTING);

    currentEvent.type = MENU_ITEM_DEPART;
    currentEvent.direction = direction;

    // hide header
    if (kHeaderHeight) {
        const AssetImage& label = *assets->header;
        vid->bg1.image(vec(0,0), label);
    }
}

/**
 * Tilting state: MENU_STATE_TILTING
 *
 * This state is active when the menu is being scrolled due to tilt and has not
 * yet hit a backstop.
 * Transitions:
 * -> Inertia when scroll goes out of bounds or cube is no longer tilted > kAccelThresholdOff.
 * Events:
 * none.
 */
inline void Menu::transToTilting()
{
    ASSERT(abs(accel.x) > kAccelThresholdOn);
}

inline void Menu::stateTilting()
{
    // normal scrolling
    const int max_x = stoppingPositionFor(numItems - 1);
    const float kInertiaThreshold = 10.f;

    velocity += (accel.x * frameclock.delta() * kTimeDilator) * velocityMultiplier();

    // clamp maximum velocity based on cube angle
    if (abs(velocity) > maxVelocity()) {
        velocity = (velocity < 0 ? 0 - maxVelocity() : maxVelocity());
    }

    // don't go past the backstop unless we have inertia
    if ((position > 0.f && velocity < 0) || (position < max_x && velocity > 0) || abs(velocity) > kInertiaThreshold) {
        position += velocity * frameclock.delta() * kTimeDilator;
    } else {
        velocity = 0;
    }
    updateBG0();
}

inline void Menu::transFromTilting()
{
    const bool outOfBounds = (position < -0.05f) || (position > kItemPixelWidth()*(numItems-1) + kEndCapPadding + 0.05f);
    if (abs(accel.x) < kAccelThresholdOff || outOfBounds) {
        changeState(MENU_STATE_INERTIA);
    }
}

/**
 * Inertia state: MENU_STATE_INERTIA
 *
 * This state is active when the menu is scrolling but either by inertia or by
 * scrolling past the backstop.
 * Transitions:
 * -> Tilting when x accelerometer exceeds kAccelThresholdOn.
 * -> Static when menu arrives at its stopping position.
 * Events:
 * none.
 */
inline void Menu::transToInertia()
{
    stopping_position = stoppingPositionFor(computeSelected());
    if (abs(accel.x) > kAccelThresholdOff) {
        tiltDirection = (kAccelScalingFactor * accel.x < 0) ? 1 : -1;
    } else {
        tiltDirection = 0;
    }
}

inline void Menu::stateInertia()
{
    checkForPress();

    const float stiffness = 0.333f;

    // do not pull to item unless tilting has stopped.
    if (abs(accel.x) < kAccelThresholdOff) {
        tiltDirection = 0;
    }
    // if still tilting, do not bounce back to the stopping position.
    if ((tiltDirection < 0 && velocity >= 0.f) || (tiltDirection > 0 && velocity <= 0.f)) {
        return;
    }

    velocity += stopping_position - position;
    velocity *= stiffness;
    position += velocity * frameclock.delta() * kTimeDilator;
    position = lerp(position, stopping_position, 0.15f);

    stateFinished = abs(velocity) < 1.0f && abs(stopping_position - position) < 0.5f;
    if (stateFinished) {
        // prevent being off by one pixel when we stop
        position = stopping_position;
    }

    updateBG0();
}

inline void Menu::transFromInertia()
{
    if (abs(accel.x) > kAccelThresholdOn &&
        !((tiltDirection < 0 && accel.x < 0.f) || (tiltDirection > 0 && accel.x > 0.f))) {
        changeState(MENU_STATE_TILTING);
    }
    if (stateFinished) { // stateFinished formerly doneTilting
        changeState(MENU_STATE_STATIC);
    }
}

/**
 * Finish state: MENU_STATE_FINISH
 *
 * This state is responsible for animating the menu out.
 * Transitions:
 * -> Start when finished and state is run an extra time. This resets and
 *          re-enters the menu.
 * Events:
 * MENU_EXIT fired after the last iteration has been painted, indicating that
 *           the menu is finished and the caller should leave its event loop.
 */
inline void Menu::transToFinish()
{
    // Prepare screen for item animation

    // We're about to switch things up in VRAM, make sure the cubes are done drawing.
    System::finish();

    // blank out the background layer
    vid->bg0.setPanning(vec(0, 0));
    vid->bg0.erase(*assets->background);

    if (assets->header) {
        Int2 vec = {0, 0};
        vid->bg0.image(vec, *assets->header);
    }
    if (assets->footer) {
        Int2 vec = { 0, kNumVisibleTilesY - assets->footer->tileHeight() };
        vid->bg0.image(vec, *assets->footer);
    }
    {
        const AssetImage* icon = items[computeSelected()].icon;
        vid->bg1.eraseMask();
        vid->bg1.fillMask(vec(0,0), icon->tileSize());
        vid->bg1.image(vec(0,0), *icon);
    }
    finishIteration = 0;
    currentEvent.type = MENU_PREPAINT;
}

inline void Menu::stateFinish()
{
    const float k = 5.f;
    int offset = 0;

    finishIteration++;
    float u = finishIteration/33.f;
    u = (1.f-k*u);
    offset = int(12*(1.f-u*u));
    vid->bg1.setPanning(vec(-kEndCapPadding, offset + kIconYOffset));
    currentEvent.type = MENU_PREPAINT;

    if (offset <= -128) {
        currentEvent.type = MENU_EXIT;
        currentEvent.item = computeSelected();
        stateFinished = true;
    }
}

inline void Menu::transFromFinish()
{
    if (stateFinished) {
        // We already animated ourselves out of a job. If we're being called again, reset the menu
        changeState(MENU_STATE_START);

        // And re-run the first iteraton of the event loop
        MenuEvent ignore;
        pollEvent(&ignore);
        /* currentEvent will be set by this second iteration of pollEvent,
         * so when we return from this function the currentEvent will be
         * propagated back to pollEvent's parameter.
         */
    }
}

/**
 * Finish state: MENU_STATE_HOP_UP
 *
 * This state is responsible for animating the menu in, after exiting a game.
 * Transitions:
 * -> Start when finished. This puts the menu back on the normal init path.
 * Events:
 * none.
 */
inline void Menu::transToHopUp()
{
    // blank out the background layer
    vid->initMode(BG0_SPR_BG1);
    vid->bg0.setPanning(vec(0, 0));
    vid->bg0.erase(*assets->background);
    
    if (assets->header) {
        Int2 vec = {0, 0};
        vid->bg0.image(vec, *assets->header);
    }
    if (assets->footer) {
        Int2 vec = { 0, kNumVisibleTilesY - assets->footer->tileHeight() };
        vid->bg0.image(vec, *assets->footer);
    }
    {
        const AssetImage* icon = items[computeSelected()].icon;
        vid->bg1.eraseMask();
        vid->bg1.fillMask(vec(0,0), icon->tileSize());
        vid->bg1.image(vec(0,0), *icon);
    }
    finishIteration = 30;
    
    hasBeenStarted = true;
}

inline void Menu::stateHopUp()
{
    const float k = 5.f;
    int offset = 0;

    finishIteration--;
    float u = finishIteration/33.f;
    u = (1.f-k*u);
    offset = int(12*(1.f-u*u));
    vid->bg1.setPanning(vec(-kEndCapPadding, offset + kIconYOffset));
    currentEvent.type = MENU_PREPAINT;

    if (offset >= 0) {
        stateFinished = true;
    }
}

inline void Menu::transFromHopUp()
{
    if (stateFinished) {
        // We're done with the animation, so jump right into the menu as normal.
        changeState(MENU_STATE_START);

        // And re-run the first iteraton of the event loop
        MenuEvent ignore;
        pollEvent(&ignore);
        /* currentEvent will be set by this second iteration of pollEvent,
         * so when we return from this function the currentEvent will be
         * propagated back to pollEvent's parameter.
         */
    }
}

/**
 * @} end addtogroup menu
 */

};  // namespace Sifteo
