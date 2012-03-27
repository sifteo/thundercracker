/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 * @file
 * @version 1.0
 *
 * @section DESCRIPTION
 * The Menu class and associated data types make it easy to create a menu.
 *
 * @section LICENSE
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MENU_H
#define _SIFTEO_MENU_H

#include <stdint.h>
#include <sifteo.h>

// C++ does not allow flexible array members in structs like C. 8 should be enough.
#define MENU_MAX_TIPS 8

namespace Sifteo {

typedef enum {
	MENU_UNEVENTFUL = 0,
	MENU_NEIGHBOR_ADD,
	MENU_NEIGHBOR_REMOVE,
	MENU_ITEM_ARRIVE,
	MENU_ITEM_DEPART,
	MENU_ITEM_PRESS,
	MENU_EXIT,
	MENU_PREPAINT
} MenuEventType;

struct MenuAssets {
	const AssetImage *background; // 1x1tl background image, repeating
	const AssetImage *footer; // ptr to 16x4tl blank footer
	const AssetImage *header; // ptr to 16x2tl blank header, optional if all items have no labels.
	const AssetImage *tips[MENU_MAX_TIPS]; // NULL-terminated array of ptrs to 16x4tl footer tips ("Choose a thing", "Tilt to scroll", "Press to select", …)
};

struct MenuItem {
	const AssetImage *icon; // ptr to 10x10tl icon, must be set to be a valid option.
	const AssetImage *label; // ptr to 16x2tl (header) label, can be NULL (defaults to blank theme header).
};

struct MenuNeighbor {
	bool operator==(const struct MenuNeighbor& rhs) const
 	{
 	   return (masterSide == rhs.masterSide)
 	       && (neighbor == rhs.neighbor)
 	       && (neighborSide == rhs.neighborSide);
 	}
 	bool operator!=(const struct MenuNeighbor& rhs) const
 	{
 	  return !operator==(rhs);
 	}

	_SYSSideID masterSide;
	_SYSCubeID neighbor;
	_SYSSideID neighborSide;
};

struct MenuEvent {
	MenuEventType type;
	union {
		struct MenuNeighbor neighbor;
		uint8_t item;
	};
};

/* Menu execution states:
 * Start: animate in menu. -> Static.
 * Static: steady state. -> Tilting, Finish.
 * Tilting: cube is being tilted. -> Inertia.
 * Inertia: menu is coasting. -> Tilting, Static.
 * Finish: item selected, animate out menu. -> Start.
 */
typedef enum {
	MENU_STATE_START,
	MENU_STATE_STATIC,
	MENU_STATE_TILTING,
	MENU_STATE_INERTIA,
	MENU_STATE_FINISH
} MenuState;

class Menu {
 public:
    Menu(Cube *, struct MenuAssets*, struct MenuItem*);
	bool pollEvent(struct MenuEvent *);
	void preventDefault();
	void reset();
	void replaceIcon(uint8_t item, const AssetImage *);
	void paintSync();
	bool itemVisible(uint8_t item);

 private:
	static const float kIconPixelWidth = 80.f;
	static const float kIconPixelHeight = 80.f;
	static const int kIconTileWidth;
	static const int kIconTileHeight;
	static const int kColumnsPerIcon;
	static const float kIconPadding = 16.f;
	static const float kPixelsPerIcon;
	static const int kEndCapPadding;
	static const float kTimeDilator = 13.1f;
	static const float kMaxSpeedMultiplier = 3.f;
	static const float kAccelScalingFactor = -0.25f;
	static const float kOneG;
	static const uint8_t kNumTilesX = 18;
	static const uint8_t kNumVisibleTilesX = 16;
	static const uint8_t kNumVisibleTilesY = 16;
	static const uint8_t kFooterHeight = 4;
	static const float kAccelThresholdOn = 1.15f;
	static const float kAccelThresholdOff = 0.85f;
	// semi-constants
	uint8_t kHeaderHeight;
	uint8_t kFooterBG1Offset;

	// external parameters and metadata
	Cube *pCube;				// cube on which the menu is being drawn
	struct MenuAssets *assets;	// theme assets of the menu
	uint8_t numTips;			// number of tips in the theme
	struct MenuItem *items;		// items in the strip
	uint8_t numItems;			// number of items in the strip
	// event breadcrumb
	struct MenuEvent currentEvent;
	// state tracking
	MenuState currentState;
	bool stateFinished;
	// paint hinting
	bool shouldPaintSync;
	// accelerometer caching
	float xaccel;
	float yaccel;
	// footer drawing
	int currentTip;
	SystemTime prevTipTime;
	// static state: event at beginning of touch only
	bool prevTouch;
	// inertial state: where to stop
	float stopping_position;
	int tiltDirection;
	// scrolling states (Inertia and Tilt): physics
	float position;			// current x position
	int prev_ut;			// tile validity tracker
	float velocity;			// current velocity
	TimeStep frameclock;	// framerate timer
	// finish state: animation iterations
	int finishIteration;
	// internal
	VidMode_BG0_SPR_BG1 canvas;
	struct MenuNeighbor neighbors[NUM_SIDES]; // menu neighbours

	// state handling
	void changeState(MenuState);
	void transToStart();
	void stateStart();
	void transFromStart();
	void transToStatic();
	void stateStatic();
	void transFromStatic();
	void transToTilting();
	void stateTilting();
	void transFromTilting();
	void transToInertia();
	void stateInertia();
	void transFromInertia();
	void transToFinish();
	void stateFinish();
	void transFromFinish();

	// event handling
	bool dispatchEvent(struct MenuEvent *ev);
	void clearEvent();
	void performDefault();
	void handleNeighborAdd();
	void handleNeighborRemove();
	void handleItemArrive();
	void handleItemDepart();
	void handleItemPress();
	void handleExit();
	void handlePrepaint();

	// library
	void detectNeighbors();
	uint8_t computeSelected();
	static unsigned unsignedMod(int, unsigned);
	void drawColumn(int);
	void drawFooter(bool force = false);
	static float stoppingPositionFor(int);
	float velocityMultiplier();
	float maxVelocity();
	static float lerp(float min, float max, float u);
	void updateBG0();
	bool itemVisibleAtCol(uint8_t item, int column);
};

// constant folding
const float Menu::kPixelsPerIcon = kIconPixelWidth + kIconPadding;
const float Menu::kOneG = abs(64 * kAccelScalingFactor);
const int Menu::kIconTileWidth = (int)(kIconPixelWidth / 8);
const int Menu::kIconTileHeight = (int)(kIconPixelHeight / 8);
const int Menu::kColumnsPerIcon = (int)(kPixelsPerIcon / 8);
const int Menu::kEndCapPadding = (kNumVisibleTilesX / 2) - (kIconTileWidth / 2);

Menu::Menu(Cube *mainCube, struct MenuAssets *aAssets, struct MenuItem *aItems)
 	: canvas(mainCube->vbuf) {
	ASSERT((int)kIconPixelWidth % 8 == 0);
	ASSERT((int)kIconPadding % 8 == 0);

	currentEvent.type = MENU_UNEVENTFUL;
	pCube = mainCube;
	changeState(MENU_STATE_START);
	currentEvent.type = MENU_UNEVENTFUL;
	items = aItems;
	assets = aAssets;

	// calculate the number of items
	uint8_t i = 0;
	while(items[i].icon != NULL) {
		ASSERT(items[i].icon->width == (int)kIconPixelWidth / 8);
		i++;
		if (items[i].label != NULL) {
			ASSERT(items[i].label->width == kNumVisibleTilesX);
			if (kHeaderHeight == 0) {
				kHeaderHeight = items[i].label->height;
			} else {
				ASSERT(items[i].label->height == kHeaderHeight);
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
		ASSERT(assets->tips[i]->width == kNumVisibleTilesX);
		ASSERT(assets->tips[i]->height == kFooterHeight);
		i++;
	}
	numTips = i;

	// sanity check the rest of the assets
	ASSERT(assets->background);
	ASSERT(assets->background->width == 1 && assets->background->height == 1);
	if (assets->footer) {
		ASSERT(assets->footer->width == kNumVisibleTilesX);
		ASSERT(assets->footer->height == kFooterHeight);
	}

	if (assets->header) {
		ASSERT(assets->header->width == kNumVisibleTilesX);
		if (kHeaderHeight == 0) {
			kHeaderHeight = assets->header->height;
		} else {
			ASSERT(assets->header->height == kHeaderHeight);
		}
	}
	kFooterBG1Offset = assets->header == NULL ? 0 : assets->header->width * assets->header->height;
	// XXX: pending fix to #8:
	ASSERT(kHeaderHeight == 0 || kHeaderHeight == 2);
}

/*
 *              _     _ _                         _   _                
 *  _ __  _   _| |__ | (_) ___   _ __ _   _ _ __ | |_(_)_ __ ___   ___ 
 * | '_ \| | | | '_ \| | |/ __| | '__| | | | '_ \| __| | '_ ` _ \ / _ \
 * | |_) | |_| | |_) | | | (__  | |  | |_| | | | | |_| | | | | | |  __/
 * | .__/ \__,_|_.__/|_|_|\___| |_|   \__,_|_| |_|\__|_|_| |_| |_|\___|
 * |_|                                                                 
 * 
 */

bool Menu::pollEvent(struct MenuEvent *ev) {
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

void Menu::preventDefault() {
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

void Menu::reset() {
	changeState(MENU_STATE_START);
}

void Menu::replaceIcon(uint8_t item, const AssetImage *icon) {
	ASSERT(item < numItems);
	items[item].icon = icon;

	if (itemVisible(item)) {
		for(int i = prev_ut; i < prev_ut + kNumTilesX; i++)
			if (itemVisibleAtCol(item, i))
				drawColumn(i);
	}

}

void Menu::paintSync() {
	// this is only relevant when caller is handling a PREPAINT event.
	ASSERT(currentEvent.type == MENU_PREPAINT);
	shouldPaintSync = true;
}

bool Menu::itemVisible(uint8_t item) {
	ASSERT(item >= 0 && item < numItems);

	for(int i = MAX(0, prev_ut - kEndCapPadding); i < prev_ut - kEndCapPadding + kNumTilesX; i++) {
		if (itemVisibleAtCol(item, i)) return true;
	}
	return false;
}

/*
 *      _        _         _                     _ _ _             
 *  ___| |_ __ _| |_ ___  | |__   __ _ _ __   __| | (_)_ __   __ _ 
 * / __| __/ _` | __/ _ \ | '_ \ / _` | '_ \ / _` | | | '_ \ / _` |
 * \__ \ || (_| | ||  __/ | | | | (_| | | | | (_| | | | | | | (_| |
 * |___/\__\__,_|\__\___| |_| |_|\__,_|_| |_|\__,_|_|_|_| |_|\__, |
 *                                                           |___/
 *
 * States are represented by three functions per state:
 * transTo$TATE() - called whenever transitioning into the state.
 * state$TATE() - the state itself, called every loop of the event pump.
 * transFrom$TATE() - called before every loop of the event pump, responsible
 *                    for transitioning to other states.
 *
 * States are identified by MENU_STATE_$TATE values in enum MenuState, and are
 * mapped to their respective functions in changeState and pollEvent.
 */

void Menu::changeState(MenuState newstate) {
	stateFinished = false;
	currentState = newstate;

	LOG(("STATE: -> "));
	switch(currentState) {
		case MENU_STATE_START:
			LOG(("start\n"));
			transToStart();
			break;
		case MENU_STATE_STATIC:
			LOG(("static\n"));
			transToStatic();
			break;
		case MENU_STATE_TILTING:
			LOG(("tilting\n"));
			transToTilting();
			break;
		case MENU_STATE_INERTIA:
			LOG(("inertia\n"));
			transToInertia();
			break;
		case MENU_STATE_FINISH:
			LOG(("finish\n"));
			transToFinish();
			break;
	}
}

/* Start state: MENU_STATE_START
 * This state is responsible for initialization of a potentially dirty menu
 * object, including setting up the video modes and animating the menu in.
 * Transitions:
 * -> Static when initialization is complete and menu becomes interactive.
 * Events:
 * none.
 */
void Menu::transToStart() {
}

void Menu::stateStart() {
	// initialize video state
	canvas.clear();
	canvas.BG1_setPanning(Vec2(0, 0));
	BG1Helper(*pCube).Flush();
    {
    	// Allocate tiles for the static upper label, and draw it.
    	if (kHeaderHeight) {
    		const AssetImage& label = items[0].label ? *items[0].label : *assets->header;
    		_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, ((1 << label.width) - 1), label.height);
    		_SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
    	}

		// Allocate tiles for the footer, and draw it.
		if (assets->footer) {
			const AssetImage& footer = *assets->footer;
    		_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + (kNumVisibleTilesY - footer.height), ((1 << footer.width) - 1), footer.height);
    		_SYS_vbuf_writei(
    			&pCube->vbuf.sys, 
    			offsetof(_SYSVideoRAM, bg1_tiles) / 2 + kFooterBG1Offset,
    		    footer.tiles, 
    		    0, 
    		    footer.width * footer.height
    		);
    	}
	}
    for (int x = -1; x < kNumTilesX - 1; x++) { drawColumn(x); }

	currentTip = 0;
	prevTipTime = SystemTime::now();
	drawFooter(true);

	canvas.set();

	shouldPaintSync = true;

	// if/when we start animating the menu into existence, set this once the work is complete
	stateFinished = true;
}

void Menu::transFromStart() {
	if (stateFinished) {
		position = 0.f;
		prev_ut = 0;

		for(int i = 0; i < NUM_SIDES; i++) {
			neighbors[i].neighborSide = SIDE_UNDEFINED;
			neighbors[i].neighbor = CUBE_ID_UNDEFINED;
			neighbors[i].masterSide = SIDE_UNDEFINED;
		}

		updateBG0();
		changeState(MENU_STATE_STATIC);
	}
}

/* Static state: MENU_STATE_STATIC
 * The resting state of the menu, the MENU_ITEM_PRESS event is fired from this
 * state on a new touch event.
 * Transitions:
 * -> Tilting when x accelerometer exceeds kAccelThresholdOn.
 * Events:
 * MENU_ITEM_ARRIVE fired when entering this state.
 * MENU_ITEM_PRESS fired when cube is touched in this state.
 * MENU_ITEM_DEPART fired when leaving this state due to tilt.
 */
void Menu::transToStatic() {
	velocity = 0;
	prevTouch = pCube->touching();

	currentEvent.type = MENU_ITEM_ARRIVE;
	currentEvent.item = computeSelected();

	// show the title of the item
	if (kHeaderHeight) {
		const AssetImage& label = items[currentEvent.item].label ? *items[currentEvent.item].label : *assets->header;
		_SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
	}
}

void Menu::stateStatic() {
	bool touch = pCube->touching();

	if (touch && !prevTouch) {
		currentEvent.type = MENU_ITEM_PRESS;
		currentEvent.item = computeSelected();
	}
	prevTouch = touch;
}

void Menu::transFromStatic() {
	if (abs(xaccel) > kAccelThresholdOn) {
		changeState(MENU_STATE_TILTING);

		currentEvent.type = MENU_ITEM_DEPART;
		currentEvent.item = computeSelected();
		
		// hide header
		if (kHeaderHeight) {
			const AssetImage& label = *assets->header;
	    	_SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
		}
	}
}

/* Tilting state: MENU_STATE_TILTING
 * This state is active when the menu is being scrolled due to tilt and has not
 * yet hit a backstop.
 * Transitions:
 * -> Inertia when scroll goes out of bounds or cube is no longer tilted > kAccelThresholdOff.
 * Events:
 * none.
 */
void Menu::transToTilting() {
	ASSERT(abs(xaccel) > kAccelThresholdOn);
}

void Menu::stateTilting() {
	// normal scrolling
	const float max_x = stoppingPositionFor(numItems - 1);
	const float kInertiaThreshold = 10.f;

	velocity += (xaccel * frameclock.delta() * kTimeDilator) * velocityMultiplier();

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

void Menu::transFromTilting() {
	const bool outOfBounds = (position < 0.f - 0.05f) || (position > kPixelsPerIcon*(numItems-1) + 0.05f);
	if (abs(xaccel) < kAccelThresholdOff || outOfBounds) {
		changeState(MENU_STATE_INERTIA);
	}
}

/* Inertia state: MENU_STATE_INERTIA
 * This state is active when the menu is scrolling but either by inertia or by
 * scrolling past the backstop.
 * Transitions:
 * -> Tilting when x accelerometer exceeds kAccelThresholdOn.
 * -> Static when menu arrives at its stopping position.
 * Events:
 * none.
 */
void Menu::transToInertia() {
	stopping_position = stoppingPositionFor(computeSelected());
	if (abs(xaccel) > kAccelThresholdOff) {
		tiltDirection = (kAccelScalingFactor * xaccel < 0) ? 1 : -1;
	} else {
		tiltDirection = 0;
	}
}

void Menu::stateInertia() {
	const float stiffness = 0.333f;

	// do not pull to item unless tilting has stopped.
	if (abs(xaccel) < kAccelThresholdOff) {
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

void Menu::transFromInertia() {
	if (abs(xaccel) > kAccelThresholdOn &&
		!((tiltDirection < 0 && xaccel < 0.f) || (tiltDirection > 0 && xaccel > 0.f))) {
		changeState(MENU_STATE_TILTING);
	}
	if (stateFinished) { // stateFinished formerly doneTilting
		changeState(MENU_STATE_STATIC);
	}
}

/* Finish state: MENU_STATE_FINISH
 * This state is responsible for animating the menu out.
 * Transitions:
 * -> Start when finished and state is run an extra time. This resets and
 *          re-enters the menu.
 * Events:
 * MENU_EXIT fired after the last iteration has been painted, indicating that
 *           the menu is finished and the caller should leave its event loop.
 */
void Menu::transToFinish() {
	// prepare screen for item animation
	// isolate the selected icon
	canvas.BG0_setPanning(Vec2(0,0));

	// blank out the background layer
	for(int row=2; row<kIconTileHeight + 2; ++row)
	for(int col=0; col<kNumVisibleTilesX; ++col) {
		canvas.BG0_drawAsset(Vec2(col, row), *assets->background);
	}
	if (assets->footer) {
		Int2 vec = { 0, kNumVisibleTilesY - assets->footer->height };
		canvas.BG0_drawAsset(vec, *assets->footer);
	}
	{
		const AssetImage* icon = items[computeSelected()].icon;
		BG1Helper overlay(*pCube);
		Int2 vec = {(kNumVisibleTilesX - icon->width) / 2, 2};
		overlay.DrawAsset(vec, *icon);
		overlay.Flush();
	}
	finishIteration = 0;
	shouldPaintSync = true;
	currentEvent.type = MENU_PREPAINT;
}

void Menu::stateFinish() {
	const float k = 5.f;
	int offset = 0;

	finishIteration++;
	float u = finishIteration/33.f;
	u = (1.f-k*u);
	offset = int(12*(1.f-u*u));
	VidMode_BG0_SPR_BG1(pCube->vbuf).BG1_setPanning(Vec2(0, offset));
	currentEvent.type = MENU_PREPAINT;

	if (offset <= -128) {
		currentEvent.type = MENU_EXIT;
		currentEvent.item = computeSelected();
		stateFinished = true;
	}
}

void Menu::transFromFinish() {
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

/*
 *                       _     _                     _ _ _             
 *   _____   _____ _ __ | |_  | |__   __ _ _ __   __| | (_)_ __   __ _ 
 *  / _ \ \ / / _ \ '_ \| __| | '_ \ / _` | '_ \ / _` | | | '_ \ / _` |
 * |  __/\ V /  __/ | | | |_  | | | | (_| | | | | (_| | | | | | | (_| |
 *  \___| \_/ \___|_| |_|\__| |_| |_|\__,_|_| |_|\__,_|_|_|_| |_|\__, |
 *                                                               |___/
 *
 * Stateful events are dispatched by setting MenuEvent.type (and
 * MenuEvent.neighbor/MenuEvent.item as appropriate) in a transTo$TATE or
 * state$TATE method and returning immediately.
 *
 * All events are immediately caught by the dispatchEvent method in pollEvent,
 * which copies it into the caller's event structure, and returns control to
 * the caller to handle the event.
 *
 * If the caller would like to prevent the default handling of the event,
 * calling Menu::preventDefault will clear the event. This is useful for
 * multistate items (the MENU_ITEM_PRESS event can replace the item's icon
 * using replaceIcon to reflect the new state instead of animating out of the
 * menu).
 * NOTE: MENU_PREPAINT should never have its default behaviour disabled as the
 * menu tracks internally if a synchronous paint is needed. If the caller needs
 * a synchronous paint for the other cubes in the system, Menu::paintSync will
 * force a synchronous paint for the current paint operation.
 *
 * Default event handling is performed at the beginning of pollEvent, before
 * re-entering the state machine.
 */

void Menu::handleNeighborAdd() {
	LOG(("Default handler: neighborAdd\n"));
	// TODO: play a sound
}

void Menu::handleNeighborRemove() {
	LOG(("Default handler: neighborRemove\n"));
	// TODO: play a sound
}

void Menu::handleItemArrive() {
	LOG(("Default handler: itemArrive\n"));
	// TODO: play a sound
}

void Menu::handleItemDepart() {
	LOG(("Default handler: itemDepart\n"));
	// TODO: play a sound
}

void Menu::handleItemPress() {
	LOG(("Default handler: itemPress\n"));
	// animate out icon
	changeState(MENU_STATE_FINISH);
}

void Menu::handleExit() {
	LOG(("Default handler: exit\n"));
	// nothing
}

void Menu::handlePrepaint() {
	if (shouldPaintSync) {
		// GFX Workaround
		System::paintSync();
		pCube->vbuf.touch();
		System::paintSync();
	} else {
		System::paint();
	}
}

void Menu::performDefault() {
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

void Menu::clearEvent() {
	currentEvent.type = MENU_UNEVENTFUL;
}

bool Menu::dispatchEvent(struct MenuEvent *ev) {
	if (currentEvent.type != MENU_UNEVENTFUL) {
		*ev = currentEvent;
		return true;
	}
	return false;
}

/*
 *  _ _ _                          
 * | (_) |__  _ __ __ _ _ __ _   _ 
 * | | | '_ \| '__/ _` | '__| | | |
 * | | | |_) | | | (_| | |  | |_| |
 * |_|_|_.__/|_|  \__,_|_|   \__, |
 *                           |___/
 */

void Menu::detectNeighbors() {
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

uint8_t Menu::computeSelected() {
	int s = (position + (kPixelsPerIcon / 2.f))/kPixelsPerIcon;
	return clamp(s, 0, numItems - 1);
}

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
void Menu::drawColumn(int x) {
	// x is the column in "global" space
    uint16_t addr = unsignedMod(x, kNumTilesX);
    x -= kEndCapPadding; // first icon is 24px inset
	const uint16_t local_x = Menu::unsignedMod(x, kColumnsPerIcon);
	const int iconId = x < 0 ? -1 : x / kColumnsPerIcon;

	// icon or blank column?
	if (local_x < kIconTileWidth && iconId >= 0 && iconId < numItems) {
		// drawing an icon column
		const AssetImage* pImg = items[x / kColumnsPerIcon].icon;
		const uint16_t *src = pImg->tiles + unsignedMod(local_x, pImg->width);
		addr += 2*kNumTilesX;
		for(int row=0; row<kIconTileHeight; ++row) {
	        _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
    	    addr += kNumTilesX;
        	src += pImg->width;

		}
	} else {
		// drawing a blank column
		VidMode_BG0 g(pCube->vbuf);
		for(int row=0; row<kIconTileHeight; ++row) {
			Int2 vec = { addr, row+2 };
			g.BG0_drawAsset(vec, *assets->background);
		}
	}
}

unsigned Menu::unsignedMod(int x, unsigned y) {
	const int z = x % (int)y;
	return z < 0 ? z+y : z;
}

void Menu::drawFooter(bool force) {
	const AssetImage& footer = numTips > 0 ? *assets->tips[currentTip] : *assets->footer;
	const float kSecondsPerTip = 4.f;

	if (numTips == 0) return;

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

float Menu::stoppingPositionFor(int selected) {
	return kPixelsPerIcon * selected;
}

float Menu::velocityMultiplier() {
	return yaccel > kAccelThresholdOff ? (1.f + (yaccel * kMaxSpeedMultiplier / kOneG)) : 1.f;
}

float Menu::maxVelocity() {
	const float kMaxNormalSpeed = 40.f;
	return kMaxNormalSpeed *
	       // x-axis linear limit
	       (abs(xaccel) / kOneG) *
	       // y-axis multiplier
	       velocityMultiplier();
}

float Menu::lerp(float min, float max, float u) {
	return min + u * (max - min);
}

void Menu::updateBG0() {
	int ui = position;
	int ut = (position < 0 ? position - 8.f : position) / 8; // special case because int rounds up when < 0

	/* XXX: we should always paintSync if we've loaded two columns, to avoid a race
	 * condition in the painting loop
	 */
	if (abs(prev_ut - ut) > 1) shouldPaintSync = true;

	while(prev_ut < ut) {
		drawColumn(++prev_ut + kNumVisibleTilesX);
	}
	while(prev_ut > ut) {
		drawColumn(--prev_ut);
	}

	canvas.BG0_setPanning(Vec2(ui, 0));
}

bool Menu::itemVisibleAtCol(uint8_t item, int column) {
	ASSERT(item >= 0 && item < numItems);
	if (column < 0) return false;

	int offset_column = column - kEndCapPadding;
	if (Menu::unsignedMod(offset_column, kColumnsPerIcon) < kIconTileWidth && offset_column / kColumnsPerIcon == item) {
		return true;
	}
	return false;
}

};  // namespace Sifteo

#endif
