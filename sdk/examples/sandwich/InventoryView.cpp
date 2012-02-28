#include "Game.h"
#include "DrawingHelpers.h"
#include "Dialog.h"

#define HOVERING_ICON_ID	0

// hacks for now
static Dialog mDialog(0);

void InventoryView::Init() {
	CORO_RESET;
	mSelected = 0;
	Vec2 tilt = Parent()->GetCube()->virtualAccel();
	mTiltX = tilt.x;
	mTiltY = tilt.y;
	mAccumX = 0;
	mAccumY = 0;
	mTouch = Parent()->GetCube()->touching();
	mAnim = 0;
	Parent()->HideSprites();
	Parent()->Graphics().BG0_drawAsset(Vec2(0,0), InventoryBackground);
	RenderInventory();
}

void InventoryView::Restore() {
	mAccumX = 0;
	mAccumY = 0;
	mTouch = Parent()->GetCube()->touching();
	Parent()->HideSprites();
	Parent()->Graphics().BG0_drawAsset(Vec2(0,0), InventoryBackground);
	RenderInventory();
}

static const char* kLabels[4] = { "top", "left", "bottom", "right" };
int t;

void InventoryView::Update(float dt) {
	bool touch = UpdateTouch();
	CORO_BEGIN;
	while(1) {
		do {
			if (pGame->AnimFrame()%2==0) {
				ComputeHoveringIconPosition();
			}
			{
				Cube::Side side = UpdateAccum();
				if (side != SIDE_UNDEFINED) {
					Vec2 pos = Vec2(mSelected % 4, mSelected >> 2) + kSideToUnit[side];
					int idx = pos.x + (pos.y<<2);
					uint8_t items[16];
					int count = pGame->GetState()->GetItems(items);
					if (idx >= 0 && idx < count) {
						mSelected = idx;
						mAnim = 0;
						RenderInventory();
					}
				}
			}
			CORO_YIELD;
		} while(!touch);

		pGame->NeedsSync();
		CORO_YIELD;
		#if GFX_ARTIFACT_WORKAROUNDS		
			pGame->NeedsSync();
			Parent()->GetCube()->vbuf.touch();
			CORO_YIELD;
			pGame->NeedsSync();
			Parent()->GetCube()->vbuf.touch();
			CORO_YIELD;
			pGame->NeedsSync();
			Parent()->GetCube()->vbuf.touch();
			CORO_YIELD;
		#endif
		mDialog = Dialog(Parent()->GetCube());
		{
			uint8_t items[16];
			int count = pGame->GetState()->GetItems(items);
			//Parent()->Graphics().setWindow(80, 48);
			Parent()->Graphics().setWindow(80+16,128-80-16);
			mDialog.Init();
			mDialog.Erase();
			mDialog.ShowAll(gItemTypeData[items[mSelected]].description);
		}
		pGame->NeedsSync();
		Parent()->GetCube()->vbuf.touch();
		CORO_YIELD;
		for(t=0; t<16; t++) {
			Parent()->Graphics().setWindow(80+15-(t),128-80-15+(t));
			mDialog.SetAlpha(t<<4);
			CORO_YIELD;
		}
		mDialog.SetAlpha(255);
		while(Parent()->GetCube()->touching()) {
			CORO_YIELD;	
		}
		System::paintSync();
		Parent()->Restore();
		mAccumX = 0;
		mAccumY = 0;
		pGame->NeedsSync();
		CORO_YIELD;
		pGame->NeedsSync();
		Parent()->GetCube()->vbuf.touch();
		CORO_YIELD;
	}


	CORO_END;
}

void InventoryView::OnInventoryChanged() {
	RenderInventory();
}

void InventoryView::RenderInventory() {
	BG1Helper overlay = Parent()->Overlay();
	const int pad = 24;
	const int innerPad = (128-pad-pad)/3;
	uint8_t items[16];
	unsigned count = pGame->GetState()->GetItems(items);
	for(unsigned i=0; i<count; ++i) {
		const int x = i % 4;
		const int y = i >> 2;
		if (i == mSelected) {
			overlay.DrawAsset(Vec2(x<<2,y<<2), InventoryReticle);
		} else {
			overlay.DrawAsset(Vec2(1 + (x<<2),1 + (y<<2)), Items, items[i]);
		}
	}
	overlay.Flush();	
	ViewMode gfx = Parent()->Graphics();
	gfx.resizeSprite(HOVERING_ICON_ID, Vec2(16, 16));
	gfx.setSpriteImage(HOVERING_ICON_ID, Items, items[mSelected]);
	ComputeHoveringIconPosition();
	pGame->NeedsSync();
}

void InventoryView::ComputeHoveringIconPosition() {
	mAnim++;
	Parent()->Graphics().moveSprite(
		HOVERING_ICON_ID, 
		8 + (mSelected%4<<5), 
		8 + ((mSelected>>2)<<5) + kHoverTable[ mAnim % HOVER_COUNT]
	);
}

Cube::Side InventoryView::UpdateAccum() {
	Vec2 tilt = Parent()->GetCube()->virtualAccel();
	mTiltX = tilt.x;
	mTiltY = tilt.y;
	const int radix = 8;
	const int threshold = 128;
	int dx = mTiltX/radix;
	int dy = mTiltY/radix;
	if (dx) {
		mAccumX += dx;
	} else {
		mAccumX = 0;
	}
	if (dy) {
		mAccumY += dy;
	} else {
		mAccumY = 0;
	}
	if (dx) {
		if (mAccumX >= threshold) {
			mAccumX %= threshold;
			return SIDE_RIGHT;
		} else if (mAccumX <= -threshold) {
			mAccumX %= threshold;
			return SIDE_LEFT;
		} 
	}
	if (dy) {
		if (mAccumY >= threshold) {
			mAccumY %= threshold;
			return SIDE_BOTTOM;
		} else if (mAccumY <= -threshold) {
			mAccumY %= threshold;
			return SIDE_TOP;
		}		
	}
	return SIDE_UNDEFINED;
}

bool InventoryView::UpdateTouch() {
	bool touch = Parent()->GetCube()->touching();
	if (touch != mTouch) {
		mTouch = touch;
		return mTouch;
	}
	return false;
}
