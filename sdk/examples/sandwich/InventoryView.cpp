#include "Game.h"
#include "DrawingHelpers.h"

#define mCanvas         (Parent()->Canvas())  
#define mHoveringSprite (Parent()->Canvas().sprites[0])  


void InventoryView::Init() {
	CORO_RESET;
	mSelected = 0;
	Int2 tilt = mCanvas.virtualAccel().xy();
	mTilt.set(tilt.x, tilt.y);
	mAnim = 0;
	Restore();
}

void InventoryView::Restore() {
	mAccum.set(0,0);
	mTouch = Parent()->GetCube().isTouching();
	mCanvas.bg0.image(vec(0,0), InventoryBackground);
	RenderInventory();
}

static const char* kLabels[4] = { "top", "left", "bottom", "right" };
int t;

void InventoryView::Update() {
	bool touch = UpdateTouch();
	CORO_BEGIN;
	while(1) {
		do {
			if (gGame.AnimFrame()%2==0) {
				ComputeHoveringIconPosition();
			}
			{
				Side side = UpdateAccum();
				if (side != NO_SIDE) {
                    Int2 pos = vec(mSelected % 4, mSelected >> 2) + Int2::unit(side);
					int idx = pos.x + (pos.y<<2);
					uint8_t items[16];
					int count = gGame.GetState()->GetItems(items);
					if (idx >= 0 && idx < count) {
						mSelected = idx;
						mAnim = 0;
						RenderInventory();
					}
				}
			}
			CORO_YIELD;
		} while(!touch);
		System::finish();
		CORO_YIELD;
		{
			uint8_t items[16];
			int count = gGame.GetState()->GetItems(items);
			mCanvas.setWindow(80+16,128-80-16);
			mDialog.Init(&mCanvas);
			mDialog.Erase();
			mDialog.ShowAll(gItemTypeData[items[mSelected]].description);
		}
		CORO_YIELD;
		for(t=0; t<16; t++) {
			mCanvas.setWindow(80+15-(t),128-80-15+(t));
			mDialog.SetAlpha(t<<4);
			CORO_YIELD;
		}
		mDialog.SetAlpha(255);
		while(Parent()->GetCube().isTouching()) {
			CORO_YIELD;	
		}
		System::finish();
		Parent()->Restore();
		mAccum.set(0,0);
		CORO_YIELD;
	}
	CORO_END;
}

void InventoryView::OnInventoryChanged() {
	RenderInventory();
}

void InventoryView::RenderInventory() {
	// TODO
	// BG1Helper overlay(*Parent()->GetCube());
	// const int pad = 24;
	// const int innerPad = (128-pad-pad)/3;
	uint8_t items[16];
	unsigned count = gGame.GetState()->GetItems(items);
	for(unsigned i=0; i<count; ++i) {
	 	const int x = i % 4;
	 	const int y = i >> 2;
	// 	if (i == mSelected) {
	// 		overlay.DrawAsset(vec(x<<2,y<<2), InventoryReticle);
	// 	} else {
	// 		overlay.DrawAsset(vec(1 + (x<<2),1 + (y<<2)), Items, items[i]);
	// 	}
	}
	// overlay.Flush();	
	mHoveringSprite.setImage(Items, items[mSelected]);
	ComputeHoveringIconPosition();
}

void InventoryView::ComputeHoveringIconPosition() {
	mAnim++;
	mHoveringSprite.move(
		8 + (mSelected%4<<5), 
		8 + ((mSelected>>2)<<5) + kHoverTable[ mAnim % HOVER_COUNT]
	);
}

Side InventoryView::UpdateAccum() {
	const int radix = 8;
	const int threshold = 128;
	Int2 tilt = mCanvas.virtualAccel().xy();
	mTilt = tilt;
	Int2 delta = tilt / radix;
	if (delta.x) {
		mAccum.x += delta.x;
	} else {
		mAccum.x = 0;
	}
	if (delta.y) {
		mAccum.y += delta.y;
	} else {
		mAccum.y = 0;
	}
	if (delta.x) {
		if (mAccum.x >= threshold) {
			mAccum.x %= threshold;
			return RIGHT;
		} else if (mAccum.x <= -threshold) {
			mAccum.x %= threshold;
			return LEFT;
		} 
	}
	if (delta.y) {
		if (mAccum.y >= threshold) {
			mAccum.y %= threshold;
			return BOTTOM;
		} else if (mAccum.y <= -threshold) {
			mAccum.y %= threshold;
			return TOP;
		}		
	}
	return NO_SIDE;
}

bool InventoryView::UpdateTouch() {
	bool touch = Parent()->GetCube().isTouching();
	if (touch != mTouch) {
		mTouch = touch;
		return mTouch;
	}
	return false;
}
