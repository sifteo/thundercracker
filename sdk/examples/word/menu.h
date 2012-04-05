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
	const AssetImage *icon; // ptr to icon, must be set to be a valid option.
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

// computed constants
#define kPixelsPerTile 8
#define kPeekTiles 1
#define kIconPixelWidth (kIconTileWidth * kPixelsPerTile)
#define kIconPixelHeight (kIconTileHeight * kPixelsPerTile)
#define kItemTileWidth (int)(((kEndCapPadding + kPixelsPerTile - 1) / kPixelsPerTile) + kIconTileWidth - kPeekTiles)
#define kItemPixelWidth (kItemTileWidth * kPixelsPerTile)

class Menu {
 public:
    Menu(Cube *, struct MenuAssets*, struct MenuItem*);
	bool pollEvent(struct MenuEvent *);
	void preventDefault();
	void reset();
	void replaceIcon(uint8_t item, const AssetImage *);
	void paintSync();
	bool itemVisible(uint8_t item);
	void setIconYOffset(uint8_t px);

 private:
	static const float kTimeDilator = 13.1f;
	static const float kMaxSpeedMultiplier = 3.f;
	static const float kAccelScalingFactor = -0.25f;
	static const float kOneG;
	static const uint8_t kNumTilesX = 18;
	static const uint8_t kNumVisibleTilesX = 16;
	static const uint8_t kNumVisibleTilesY = 16;
	static const float kAccelThresholdOn = 1.15f;
	static const float kAccelThresholdOff = 0.85f;
	static const uint8_t kDefaultIconYOffset = 16;
	// instance-constants
	uint8_t kHeaderHeight;
	uint8_t kFooterHeight;
	uint8_t kFooterBG1Offset;
	int8_t kIconYOffset;
	uint8_t kIconTileWidth;
	uint8_t kIconTileHeight;
	int8_t kEndCapPadding;

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
    float prevTipTime;
	// static state: event at beginning of touch only
	bool prevTouch;
	// inertial state: where to stop
	int stopping_position;
	int tiltDirection;
	// scrolling states (Inertia and Tilt): physics
	float position;			// current x position
	int prev_ut;			// tile validity tracker
	float velocity;			// current velocity
    float frameclock;	// framerate timer
    float frameclockPrev;	// framerate timer
    float dt;	// framerate timer
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
	int stoppingPositionFor(int);
	float velocityMultiplier();
	float maxVelocity();
	static float lerp(float min, float max, float u);
	void updateBG0();
	bool itemVisibleAtCol(uint8_t item, int column);
	uint8_t itemAtCol(int column);
	int computeCurrentTile();
};


// computed constants
/*
#undef kPixelsPerTile
#undef kPeekTiles
#undef kIconPixelWidth
#undef kIconPixelHeight
#undef kItemTileWidth
#undef kItemPixelWidth
*/
};  // namespace Sifteo

#endif
