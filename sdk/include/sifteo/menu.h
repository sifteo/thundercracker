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
#include <sifteo/macros.h>

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
	const AssetImage *bg; // 1x1tl background image, repeating
	const AssetImage *footer; // ptr to 16x4tl blank footer
	const AssetImage *header; // ptr to 16x2tl blank header
	const AssetImage *tips[MENU_MAX_TIPS]; // NULL-terminated array of ptrs to 16x4tl footer tips ("Choose a thing", "Tilt to scroll", "Press to select", …)
};

struct MenuItem {
	const AssetImage *icon; // ptr to 10x10tl icon, must be set to be a valid option.
	const AssetImage *label; // ptr to 16x2tl (header) label, can be NULL (defaults to blank header).
};

struct MenuEvent {
	MenuEventType type;
	union {
		Cube *neighbor;
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
	void pollEvent(struct MenuEvent *);
	void preventDefault();
	void reset();
	void replaceIcon(uint8_t, AssetImage *);
	void paintSync();
	
 private:
	static const float kIconWidth = 80.f;
	static const float kIconPadding = 16.f;
	static const float kPixelsPerIcon;
	static const float kMaxSpeedMultiplier = 3.f;
	static const float kAccelScalingFactor = -0.25f;
	static const float kOneG;
	static const uint8_t kNumTilesX = 18;
	static const uint8_t kNumVisibleTilesX = 16;
	static const uint8_t kNumVisibleTilesY = 16;
	static const uint8_t kFooterHeight = 4;
	static const uint8_t kHeaderHeight = 2;
	static const float kAccelThresholdOn = 1.15f;
	static const float kAccelThresholdOff = 0.85f;

	// external parameters and metadata
	Cube *pCube;
	struct MenuAssets *assets;
	uint8_t numTips;
	struct MenuItem *items;
	uint8_t numItems;
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
	float prevTipTime;
	// static state: event at beginning of touch only
	bool prevTouch;
	// inertial state: where to stop
	float stopping_position;
	// scrolling states (Inertia and Tilt)
	float position;
	float dt;
	float lastPaint;
	int prev_ut;
	float velocity;
	
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
	uint8_t computeSelected();
	static unsigned unsignedMod(int, unsigned);
	void drawColumn(int);
	void drawFooter();
	static float stoppingPositionFor(int);
	float velocityMultiplier();
	float maxVelocity();
	static float lerp(float min, float max, float u);
	void updateBG0();
};

// constant folding
const float Menu::kPixelsPerIcon = kIconWidth + kIconPadding;
const float Menu::kOneG = fabs(64 * kAccelScalingFactor);

Menu::Menu(Cube *mainCube, struct MenuAssets *aAssets, struct MenuItem *aItems) {
	ASSERT((int)kIconWidth % 8 == 0);
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
		ASSERT(items[i].icon->width == (int)kIconWidth / 8);
		i++;
		if(items[i].label != NULL) {
			ASSERT(items[i].label->width == kNumVisibleTilesX);
			ASSERT(items[i].label->height == kHeaderHeight);
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
	ASSERT(assets->bg);
	ASSERT(assets->bg->width == assets->bg->height == 1);
	ASSERT(assets->footer);
	ASSERT(assets->footer->width == kNumVisibleTilesX);
	ASSERT(assets->footer->height == kFooterHeight);
	ASSERT(assets->header);
	ASSERT(assets->header->width == kNumVisibleTilesX);
	ASSERT(assets->header->height == kHeaderHeight);
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

void Menu::pollEvent(struct MenuEvent *ev) {
	// handle/clear pending events
	if(currentEvent.type != MENU_UNEVENTFUL) {
		performDefault();
	}
	
	// keep track of time so if our framerate changes, we don't change apparent speed
	float now = System::clock();
	const float kTimeDilator = 13.1f;
	dt = (now - lastPaint) * kTimeDilator;
	
	// TODO: neighbour added/lost?
	
	
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
	if(dispatchEvent(ev)) {
		return;
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
		default:
			break;
	}
	if(dispatchEvent(ev)) {
		return;
	}

	// no special events, paint a frame.
	drawFooter();
	currentEvent.type = MENU_PREPAINT;
	dispatchEvent(ev);
	lastPaint = now;
	return;
}

void Menu::preventDefault() {
	// paints shouldn't be prevented--the caller doesn't know whether to paintSync or paint.
	ASSERT(currentEvent.type != MENU_PREPAINT);
	clearEvent();
}

void Menu::reset() {
	changeState(MENU_STATE_START);
}

void Menu::replaceIcon(uint8_t item, AssetImage *icon) {
	ASSERT(item < numItems);
	items[item].icon = icon;
	// TODO: force-redraw affected area
}

void Menu::paintSync() {
	// this is only relevant when caller is handling a PREPAINT event.
	ASSERT(currentEvent.type == MENU_PREPAINT);
	shouldPaintSync = true;
}

/*
 *      _        _         _                     _ _ _             
 *  ___| |_ __ _| |_ ___  | |__   __ _ _ __   __| | (_)_ __   __ _ 
 * / __| __/ _` | __/ _ \ | '_ \ / _` | '_ \ / _` | | | '_ \ / _` |
 * \__ \ || (_| | ||  __/ | | | | (_| | | | | (_| | | | | | | (_| |
 * |___/\__\__,_|\__\___| |_| |_|\__,_|_| |_|\__,_|_|_|_| |_|\__, |
 *                                                           |___/
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
	currentTip = 0;
	prevTipTime = System::clock();
}

void Menu::stateStart() {
	// initialize video state
	_SYS_vbuf_pokeb(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_BG1);
	VidMode_BG0 canvas(pCube->vbuf);
	canvas.clear();
	VidMode_BG0_SPR_BG1(pCube->vbuf).BG1_setPanning(Vec2(0, 0));
	BG1Helper(*pCube).Flush();
    {
    	// Allocate tiles for the static upper label, and draw it.
    	const AssetImage& label = *items[0].label;
    	_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, ((1 << label.width) - 1), label.height);
    	_SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);

		// Allocate tiles for the footer, and draw it.
		const AssetImage& footer = *assets->footer;
    	_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + (kNumVisibleTilesY - footer.height), ((1 << footer.width) - 1), footer.height);
    	_SYS_vbuf_writei(
    		&pCube->vbuf.sys, 
    		offsetof(_SYSVideoRAM, bg1_tiles) / 2 + label.width * label.height,
    	    footer.tiles, 
    	    0, 
    	    footer.width * footer.height
    	);
	}
    for (int x = -1; x < kNumTilesX - 1; x++) { drawColumn(x); }
    drawFooter();

	// if/when we start animating the menu into existence, set this once the work is complete
	stateFinished = true;
}

void Menu::transFromStart() {
	if (stateFinished) {
		position = 0.f;
		prev_ut = 0;
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
	
	// show the title of the game
	const AssetImage& label = *items[currentEvent.item].label;
    _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
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
	if (fabs(xaccel) > kAccelThresholdOn) {
		changeState(MENU_STATE_TILTING);

		currentEvent.type = MENU_ITEM_DEPART;
		currentEvent.item = computeSelected();
		
		// hide header
		const AssetImage& label = *assets->header;
	    _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
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
void Menu::transToTilting() {}

void Menu::stateTilting() {
	// normal scrolling
	const float max_x = stoppingPositionFor(numItems - 1);
	const float kInertiaThreshold = 10.f;
	
	velocity += (xaccel * dt) * velocityMultiplier();
	
	// clamp maximum velocity based on cube angle
	if(fabs(velocity) > maxVelocity()) {
		velocity = (velocity < 0 ? 0 - maxVelocity() : maxVelocity());
	}
	
	// don't go past the backstop unless we have inertia
	if((position > 0.f && velocity < 0) || (position < max_x && velocity > 0) || fabs(velocity) > kInertiaThreshold) {
	    position += velocity * dt;
	} else {
	    velocity = 0;
	}
	updateBG0();
}

void Menu::transFromTilting() {
	const bool outOfBounds = (position < 0.f - 0.05f) || (position > kPixelsPerIcon*(numItems-1) + 0.05f);
	if (fabs(xaccel) < kAccelThresholdOff || outOfBounds) {
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
}

void Menu::stateInertia() {
	const float stiffness = 0.333f;
	
	velocity += stopping_position - position;
	velocity *= stiffness;
	position += velocity * dt;
	position = lerp(position, stopping_position, 0.15f);
	stateFinished = fabs(velocity) < 1.0f && fabs(stopping_position - position) < 0.5f;
	if(stateFinished) {
		// prevent being off by one pixel when we stop
		position = stopping_position;
	}
	updateBG0();
}

void Menu::transFromInertia() {
	if (fabs(xaccel) > kAccelThresholdOn) {
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
void Menu::transToFinish() {}

void Menu::stateFinish() {
	// TODO: animate out first. then:
	currentEvent.type = MENU_EXIT;
	currentEvent.item = computeSelected();
	stateFinished = true;
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
 */

void Menu::handleNeighborAdd() {
	LOG(("Default handler: neighborAdd\n"));
	// play a sound
}

void Menu::handleNeighborRemove() {
	LOG(("Default handler: neighborRemove\n"));
	// play a sound
}

void Menu::handleItemArrive() {
	LOG(("Default handler: itemArrive\n"));
	// play a sound
}

void Menu::handleItemDepart() {
	LOG(("Default handler: itemDepart\n"));
	// play a sound
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
//	LOG(("Default handler: prepaint\n"));
	if(shouldPaintSync) {
		LOG(("paint: sync\n"));
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
if(currentEvent.type != MENU_UNEVENTFUL && currentEvent.type != MENU_PREPAINT) LOG(("Dispatch event: %d\n", currentEvent.type));
	if(currentEvent.type != MENU_UNEVENTFUL) {
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

uint8_t Menu::computeSelected() {
	int s = (position + (kPixelsPerIcon / 2.f))/kPixelsPerIcon;
	return clamp(s, 0, numItems - 1);
}

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
void Menu::drawColumn(int x) {
	const int kColumnsPerIcon = (int)(kPixelsPerIcon / 8); // columns taken up by the icon plus padding
	
	// x is the column in "global" space
    uint16_t addr = unsignedMod(x, kNumTilesX);
    x -= 3; // first icon is 24px inset
	const uint16_t local_x = Menu::unsignedMod(x, kColumnsPerIcon);
	const int iconId = x < 0 ? -1 : x / kColumnsPerIcon;

	// icon or blank column?
	if (local_x < 10 && iconId >= 0 && iconId < numItems) {
		// drawing an icon column
		const AssetImage* pImg = items[x / kColumnsPerIcon].icon;
		const uint16_t *src = pImg->tiles + unsignedMod(local_x, pImg->width);
		addr += 2*kNumTilesX;
		for(int row=0; row<10; ++row) {
	        _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
    	    addr += kNumTilesX;
        	src += pImg->width;

		}
	} else {
		// drawing a blank column
		VidMode_BG0 g(pCube->vbuf);
		for(int row=0; row<10; ++row) {
			g.BG0_drawAsset(Vec2(addr, row+2), *assets->bg);
		}
	}
}

unsigned Menu::unsignedMod(int x, unsigned y) {
//LOG(("%s\n", __FUNCTION__));
	const int z = x % (int)y;
	return z < 0 ? z+y : z;
}

void Menu::drawFooter() {
	const AssetImage& footer = numTips > 0 ? *assets->tips[currentTip] : *assets->footer;
	const float kSecondsPerTip = 4.f;
	float time = System::clock();
	if (time - prevTipTime > kSecondsPerTip) {
		prevTipTime = time - fmodf(time - prevTipTime, kSecondsPerTip);

		if (numTips > 0) {
			currentTip = (currentTip+1) % numTips;
		}
		
        _SYS_vbuf_writei(
        	&pCube->vbuf.sys, 
        	offsetof(_SYSVideoRAM, bg1_tiles) / 2 + assets->header->width * assets->header->height,
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
	       (fabs(xaccel) / kOneG) *
	       // y-axis multiplier
	       velocityMultiplier();
}

float Menu::lerp(float min, float max, float u) {
	return min + u * (max - min);
}

void Menu::updateBG0() {
	VidMode_BG0 canvas(pCube->vbuf);
	
	int ui = position;
	int ut = position / 8;
	if(abs(prev_ut - ut) > 0) {
		shouldPaintSync = true;
	}
	
	while(prev_ut < ut) {
		drawColumn(prev_ut + 17);
		prev_ut++;
	}
	while(prev_ut > ut) {
		drawColumn(prev_ut - 2);
		prev_ut--;
	}
	
	canvas.BG0_setPanning(Vec2(ui, 0));
}

};  // namespace Sifteo

#endif
