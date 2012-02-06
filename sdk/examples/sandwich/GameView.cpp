#include "Game.h"

Cube* GameView::GetCube() const {
	return gCubes + (this - pGame->ViewBegin());
}

Cube::ID GameView::GetCubeID() const {
	return this - pGame->ViewBegin();
}

bool GameView::Touched() const {
  return !mFlags.prevTouch && GetCube()->touching();
}

void GameView::Init() {
	mFlags.subview = (this == pGame->ViewBegin()) ? VIEW_ROOM : VIEW_IDLE;
	if (IsShowingRoom()) {
		mSubview.room.Init(pGame->GetMap()->GetRoomId(pGame->GetPlayer()->Location()));
	} else {
		mSubview.idle.Init();
	}
}

void GameView::HideSprites() {
	VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
	for(unsigned i=0; i<8; ++i) {
		mode.hideSprite(i);
	}
}

void GameView::Restore() {
	switch(mFlags.subview) {
	case VIEW_IDLE:
		mSubview.idle.Restore();
		break;
	case VIEW_ROOM:
		mSubview.room.Restore();
		break;
	case VIEW_INVENTORY:
		mSubview.inventory.Restore();
		break;
	}
}

void GameView::Update() {
	mFlags.prevTouch = GetCube()->touching();
	switch(mFlags.subview) {
	case VIEW_IDLE:
		mSubview.idle.Update();
		break;
	case VIEW_ROOM:
		mSubview.room.Update();
		break;
	case VIEW_INVENTORY:
		mSubview.inventory.Update();
		break;
	}
}
  
bool GameView::ShowLocation(Vec2 loc) {
	const Map* m = pGame->GetMap();
	if (!pGame->GetMap()->Contains(loc)) {
		if (IsShowingRoom()) {
			HideLocation();
		}
		return false;
	} else {
		unsigned rid = pGame->GetMap()->GetRoomId(loc);
		if (!IsShowingRoom() || mSubview.room.GetRoom()->Id() != rid) {
			mFlags.subview = VIEW_ROOM;
			mSubview.room.Init(rid);
		}
		return true;
	}
}

bool GameView::HideLocation() {
	if (IsShowingRoom()) {
		mFlags.subview = VIEW_IDLE;
		mSubview.idle.Init();
		return true;
	}
	return false;
}

void GameView::RefreshInventory() {
  if (mFlags.subview == VIEW_IDLE) {
  	mSubview.idle.OnInventoryChanged();
  }
}

GameView* GameView::VirtualNeighborAt(Cube::Side side) const {
  Cube::ID neighbor = GetCube()->virtualNeighborAt(side);
  return neighbor == CUBE_ID_UNDEFINED ? 0 : pGame->ViewAt(neighbor-CUBE_ID_BASE);
}

//----------------------------------------------------------------------
// ACCELEROMETER SHTUFF
//----------------------------------------------------------------------

#ifdef SIFTEO_SIMULATOR
#define TILT_THRESHOLD 50
#else
#define TILT_THRESHOLD 35
#endif

Cube::Side GameView::VirtualTiltDirection() const {
  Vec2 accel = GetCube()->virtualAccel();
  if (accel.y < -TILT_THRESHOLD) {
    return 0;
  } else if (accel.x < -TILT_THRESHOLD) {
    return 1;
  } else if (accel.y > TILT_THRESHOLD) {
    return 2;
  } else if (accel.x > TILT_THRESHOLD) {
    return 3;
  } else {
    return -1;
  }
}
