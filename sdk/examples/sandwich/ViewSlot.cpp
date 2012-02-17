#include "Game.h"

// the whole way that I "search for inventory views" kinda sucks - should cache a pointer of something...

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
	ViewMode mode = Graphics();
	mode.set();
	mode.clear();
  	mode.setWindow(0, 128);
	mView.idle.Init();
	pGame->NeedsSync();
}

void ViewSlot::HideSprites() {
	ViewMode mode = Graphics();
	for(unsigned i=0; i<8; ++i) {
		mode.hideSprite(i);
	}
}

void ViewSlot::Restore() {
	ViewMode mode = Graphics();
	mode.set();
	mode.clear();
  	mode.setWindow(0, 128);
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
	pGame->NeedsSync();
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
	if (!pGame->GetMap()->Contains(loc)) {
		if (IsShowingRoom()) {
			HideLocation();
		}
	} else {
		unsigned rid = pGame->GetMap()->GetRoomId(loc);
		if (!IsShowingRoom() || mView.room.GetRoom()->Id() != rid) {
			if (mFlags.view == VIEW_INVENTORY) {
				for (ViewSlot* p=pGame->ViewBegin(); p!=pGame->ViewEnd(); ++p) {
					if (p->ViewType() == VIEW_IDLE) {
						p->mFlags.view = VIEW_INVENTORY;
						p->mView.inventory.Init();
						break;
					}
				}
			}
			mFlags.view = VIEW_ROOM;
			mView.room.Init(rid);
			pGame->NeedsSync();
			return true;
		}
	}
	return false;
}

bool ViewSlot::HideLocation() {
	if (IsShowingRoom()) {

		if (pGame->GetState()->HasAnyItems()) {
			bool invShowing = false;
			for(ViewSlot* p=pGame->ViewBegin(); p!=pGame->ViewEnd(); ++p) {
				if ((invShowing = (p->ViewType() == VIEW_INVENTORY))) { break; }
			}
			if (invShowing) {
				mFlags.view = VIEW_IDLE;
				mView.idle.Init();
				pGame->NeedsSync();
			} else {
				mFlags.view = VIEW_INVENTORY;
				mView.inventory.Init();
				pGame->NeedsSync();
			}
		} else {
			mFlags.view = VIEW_IDLE;
			mView.idle.Init();
			pGame->NeedsSync();
		}
		return true;
	}
	return false;
}

void ViewSlot::ShowInventory() {
	if (mFlags.view != VIEW_INVENTORY) {
		mFlags.view = VIEW_INVENTORY;
		mView.inventory.Init();
		pGame->NeedsSync();
	}
}

void ViewSlot::RefreshInventory() {
  if (mFlags.view == VIEW_INVENTORY) {
  	if (!pGame->GetState()->HasAnyItems()) {
		mFlags.view = VIEW_IDLE;
		mView.idle.Init();
		pGame->NeedsSync();
  	} else {
  		mView.inventory.OnInventoryChanged();
  	}
  } else if (mFlags.view == VIEW_IDLE && pGame->GetState()->HasAnyItems()) { 
	bool invShowing = false;
	for(ViewSlot* p=pGame->ViewBegin(); p!=pGame->ViewEnd(); ++p) {
		if ((invShowing = p->ViewType() == VIEW_INVENTORY)) { break; }
	}
	if (!invShowing) {
		mFlags.view = VIEW_INVENTORY;
		mView.inventory.Init();
		pGame->NeedsSync();
  	}
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
