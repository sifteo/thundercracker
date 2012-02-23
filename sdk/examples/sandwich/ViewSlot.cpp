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

void ViewSlot::SetView(unsigned viewId, unsigned rid) {
	mFlags.view = viewId;
	if (GetCube()->vbuf.peekb(offsetof(_SYSVideoRAM, mode)) != _SYS_VM_BG0_SPR_BG1) {
		ViewMode gfx = Graphics();
		gfx.setWindow(0,128);
		gfx.init();
	}
	switch(viewId) {
		case VIEW_IDLE:
			mView.idle.Init();
			break;
		case VIEW_ROOM:
			mView.room.Init(rid);
			break;
		case VIEW_INVENTORY:
			mView.inventory.Init();
			break;
	}
	#if GFX_ARTIFACT_WORKAROUNDS
		pGame->Paint(true);
		GetCube()->vbuf.touch();
		pGame->Paint(true);
	#endif	
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

void ViewSlot::Update(float dt) {
	mFlags.prevTouch = GetCube()->touching();
	switch(mFlags.view) {
	case VIEW_IDLE:
		mView.idle.Update(dt);
		break;
	case VIEW_ROOM:
		mView.room.Update(dt);
		break;
	case VIEW_INVENTORY:
		mView.inventory.Update(dt);
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
						p->SetView(VIEW_INVENTORY);
						break;
					}
				}
			}
			SetView(VIEW_ROOM, rid);
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
				SetView(VIEW_IDLE);
			} else {
				SetView(VIEW_INVENTORY);
			}
		} else {
			SetView(VIEW_IDLE);
		}
		return true;
	}
	return false;
}

void ViewSlot::ShowInventory() {
	if (mFlags.view != VIEW_INVENTORY) {
		SetView(VIEW_INVENTORY);
	}
}

void ViewSlot::RefreshInventory() {
  if (mFlags.view == VIEW_INVENTORY) {
  	if (!pGame->GetState()->HasAnyItems()) {
  		SetView(VIEW_IDLE);
  	} else {
  		mView.inventory.OnInventoryChanged();
  	}
  } else if (mFlags.view == VIEW_IDLE && pGame->GetState()->HasAnyItems()) { 
	bool invShowing = false;
	for(ViewSlot* p=pGame->ViewBegin(); p!=pGame->ViewEnd(); ++p) {
		if ((invShowing = p->ViewType() == VIEW_INVENTORY)) { break; }
	}
	if (!invShowing) {
		SetView(VIEW_INVENTORY);
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
