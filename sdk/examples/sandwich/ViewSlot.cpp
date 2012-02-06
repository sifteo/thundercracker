#include "Game.h"

Cube* ViewSlot::GetCube() const {
	return gCubes + (this - pGame->ViewBegin());
}

Cube::ID ViewSlot::GetCubeID() const {
	return this - pGame->ViewBegin();
}

bool ViewSlot::Touched() const {
  return !mFlags.prevTouch && GetCube()->touching();
}

void ViewSlot::Init() {
	mFlags.view = VIEW_IDLE;
	mView.idle.Init();
}

void ViewSlot::HideSprites() {
	VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
	for(unsigned i=0; i<8; ++i) {
		mode.hideSprite(i);
	}
}

void ViewSlot::Restore() {
	switch(mFlags.view) {
	case VIEW_IDLE:
		mView.idle.Restore();
		break;
	case VIEW_ROOM:
		mView.room.Restore();
		break;
	case VIEW_INVENTORY:
		mView.inventory.Restore();
		break;
	}
}

void ViewSlot::Update() {
	mFlags.prevTouch = GetCube()->touching();
	switch(mFlags.view) {
	case VIEW_IDLE:
		mView.idle.Update();
		break;
	case VIEW_ROOM:
		mView.room.Update();
		break;
	case VIEW_INVENTORY:
		mView.inventory.Update();
		break;
	}
}
  
bool ViewSlot::ShowLocation(Vec2 loc) {
	const Map* m = pGame->GetMap();
	if (!pGame->GetMap()->Contains(loc)) {
		if (IsShowingRoom()) {
			HideLocation();
		}
		return false;
	} else {
		unsigned rid = pGame->GetMap()->GetRoomId(loc);
		if (!IsShowingRoom() || mView.room.GetRoom()->Id() != rid) {
			mFlags.view = VIEW_ROOM;
			mView.room.Init(rid);
		}
		return true;
	}
}

bool ViewSlot::HideLocation() {
	if (IsShowingRoom()) {
		mFlags.view = VIEW_IDLE;
		mView.idle.Init();
		return true;
	}
	return false;
}

void ViewSlot::RefreshInventory() {
  if (mFlags.view == VIEW_IDLE) {
  	mView.idle.OnInventoryChanged();
  }
}

ViewSlot* ViewSlot::VirtualNeighborAt(Cube::Side side) const {
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

Cube::Side ViewSlot::VirtualTiltDirection() const {
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
