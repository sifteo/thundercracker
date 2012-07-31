/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "ui_menu.h"
#include "ui_coordinator.h"
#include "vram.h"
#include "cubeslots.h"
#include "cube.h"

// Assets
extern const uint16_t MenuBackground_data[];

// Menu settings
namespace {
	static const unsigned TILE = 8;

    static const unsigned kIconTileWidth = 5;
    static const unsigned kIconTileHeight = 5;

    static const unsigned kIconSpacing = 8;
    static const unsigned kIconYOffset = 1;
    static const unsigned kLabelYOffset = 7;

    static const unsigned kNumTilesX = 18;
    static const unsigned kNumVisibleTilesX = 16;
    static const unsigned kNumTilesY = 9;
    static const unsigned kEndPadding = 4 * TILE;
    static const unsigned kCenterPixelX = (kNumVisibleTilesX - kIconTileWidth) * TILE / 2;

    static const unsigned kWindowHeight = kNumTilesY * TILE;
    static const unsigned kWindowBegin = (128 - kWindowHeight) / 2;
    static const unsigned kWindowBorder = 3;

    static const int kAccelHysteresisMin = 8;
    static const int kAccelHysteresisMax = 18;
    static const float kAccelScale = -0.1f;
    static const float kMaxVelocity = 4.0f;

    static const unsigned kInactivePalette = (11 ^ 6) << 10;
    static const unsigned kTextPalette = 11 << 10;
}

void UIMenu::init(unsigned defaultItem)
{
	if (!uic.isAttached())
		return;

	// Paint entire screen with background tile (We'll see it during the hop animation)
	uint16_t bg = _SYS_TILE77(MenuBackground_data[1]);
	for (unsigned i = 0; i < _SYS_VRAM_BG0_WIDTH * _SYS_VRAM_BG0_WIDTH; ++i)
		VRAM::poke(uic.avb.vbuf, i, bg);

	// Set up default video state
	VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_ROM);
	VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, first_line), kWindowBegin);
	VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, num_lines), kWindowHeight);

	// We won't transition to STATIC until we verify that isTouching() == false
	state = S_SETTLING;

	setActiveItem(defaultItem);
	position = itemCenterPosition(defaultItem);

	drawAll();
	uic.setPanX(position);
}

void UIMenu::animate()
{
	if (!uic.isAttached())
		return;
	CubeSlot &cube = CubeSlots::instances[uic.avb.cube];

    /*
     * Sample accelerometer, with hysteresis
     */

    _SYSByte4 accel = cube.getAccelState();
	int accelThreshold = state == S_TILTING ? kAccelHysteresisMin : kAccelHysteresisMax;
	bool isTilting = accel.x > accelThreshold || accel.x < -accelThreshold;

    /*
     * Update state machine
     */

    switch (state) {

	    case S_SETTLING:
	    	// Settling: Pull toward and activate the nearest item, if the accelerometer is mostly centered
	    	if (isTilting) {
	    		state = S_TILTING;
	    	} else {
	    		float distance = itemCenterPosition(activeItem) - position;
				updatePosition(distance * 0.1f);
	    		if (distance < 0.1f && distance > -0.1f && !cube.isTouching()) {
	    			state = S_STATIC;
	    		}
	    	}
	    	break;

	   	case S_TILTING:
			// Tilting: Slide in the direction of the accelerometer
			updatePosition(accel.x * kAccelScale);
			if (!isTilting) {
				state = S_SETTLING;
			}
			break;

		case S_STATIC:
			// Static: Menu isn't moving, cube wasn't being touched. Has that changed?
			if (isTilting) {
				state = S_TILTING;
			} else if (cube.isTouching()) {
				/*
				 * Entering 'finishing' state. We adjust the window so that the item
				 * hops 'behind' the small border at the bottom of the menu, and we
				 * force everything to redraw so we can hide the inactive icons.
				 */
				activeItem = -1;
				hopFrame = 0;
				state = S_FINISHING;
				VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, first_line), kWindowBegin + kWindowBorder);
				VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, num_lines), kWindowHeight - kWindowBorder*2);
				updateHop();
			}
			break;

		case S_FINISHING:
			// Finishing: Selected item 'hops' down.
			updateHop();
			break;

		default:
			break;
	}

	/*
	 * Update graphics
	 */

    unsigned newActiveItem = nearestItem();

    if (newActiveItem == activeItem) {
    	// Just draw the new portion of the screen that's scrolling in
	    drawColumns();
	} else {
		// Active item changed, redraw everything
		setActiveItem(newActiveItem);
		drawAll();
	}

	uic.setPanX(position);
}

void UIMenu::setActiveItem(unsigned n)
{
	activeItem = n;
	labelWidth = n < numItems ? strlen(items[n].label) : 0;
}

int UIMenu::itemCenterPosition(unsigned n)
{
    return kIconSpacing * TILE * n - kCenterPixelX;
}

unsigned UIMenu::nearestItem()
{
    const int pixelSpacing = kIconSpacing * TILE;
    int item = (position + kCenterPixelX + pixelSpacing/2) / pixelSpacing;
    return MAX(0, MIN(numItems - 1, item));
}

void UIMenu::updatePosition(float velocity)
{
	if (velocity >  kMaxVelocity) velocity = kMaxVelocity;
	if (velocity < -kMaxVelocity) velocity = -kMaxVelocity;

	position += velocity;

    int leftLimit = -kEndPadding - kCenterPixelX;
    int rightLimit = kIconSpacing * TILE * (numItems - 1) + kEndPadding - kCenterPixelX;
    int pixelPosition = position;
    if (pixelPosition < leftLimit)  position = leftLimit;
    if (pixelPosition > rightLimit) position = rightLimit;
}

void UIMenu::updateHop()
{
	// Update the vertical 'hop' animation (Y panning)

	static const int8_t hop[] = {
		// [int(i*1.5 - i*i*0.1 + 0.5) for i in range(35)]
		1, 3, 4, 4, 5, 5, 6, 6, 5, 5, 4, 4, 3, 1, 0, -1, -2, -4, -7,
		-9, -12, -14, -17, -21, -24, -28, -31, -35, -40, -44, -49, -53, -58, -64
	};

	ASSERT(hopFrame < arraysize(hop));
	uic.setPanY(kWindowBorder + hop[hopFrame]);

	if (++hopFrame == arraysize(hop))
		state = S_DONE;
}

void UIMenu::drawAll()
{
	int tilePosition = int(position) / int(TILE);
	prevTilePosition = tilePosition;
	for (int i = -1; i < int(kNumVisibleTilesX) + 1; ++i) {
		drawColumn(tilePosition + i);
	}
}

void UIMenu::drawColumns()
{
    // Fill in tiles ahead of the direction of scrolling
    int tilePosition = int(position) / int(TILE);
    while (tilePosition > prevTilePosition) {
        drawColumn(++prevTilePosition + kNumVisibleTilesX);
    }
    while (tilePosition < prevTilePosition) {
        drawColumn(--prevTilePosition - 1);
    }
}

void UIMenu::drawColumn(int x)
{
	// 'x' is a column index, in tiles, from the origin of our virtual coordinate
	// system. It is mapped modulo-18 onto BG0.

	unsigned addr = umod(x, kNumTilesX);

	for (unsigned y = 0; y < kNumTilesY; ++y) {
		// Don't draw borderes when finishing. Otherwise, draw the whole background.
		uint16_t tile = state == S_FINISHING ? MenuBackground_data[1] : MenuBackground_data[y];

		unsigned iconIndex = x / kIconSpacing;
		unsigned iconX = x % kIconSpacing;
		unsigned iconY = y - kIconYOffset;

		if (iconX < kIconTileWidth && iconY < kIconTileHeight && iconIndex < numItems) {
			bool active = iconIndex == activeItem;

			if (state != S_FINISHING || active) {
				tile = items[iconIndex].icon[iconX + iconY * kIconTileWidth];
				if (!active)
					tile ^= kInactivePalette;
			}
		}

		if (y == kLabelYOffset && labelWidth) {
			// Drawing the label text

			unsigned labelX = x - kIconSpacing * activeItem + (labelWidth - kIconTileWidth) / 2;
			if (labelX < labelWidth) {
				tile = (items[activeItem].label[labelX] - ' ') ^ kTextPalette;
			}
		}

        VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(tile));
        addr += kNumTilesX;
	}
}