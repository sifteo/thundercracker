#include "Game.h"

ViewSlot* pInventory = 0;
ViewSlot* pMinimap = 0;

Cube* ViewSlot::GetCube() const {
	return gCubes + (this - gGame.ViewBegin());
}

Cube::ID ViewSlot::GetCubeID() const {
	return this - gGame.ViewBegin();
}

bool ViewSlot::Touched() const {
  return !mFlags.prevTouch && GetCube()->touching();
}

void ViewSlot::Init() {
	ViewMode mode(GetCube()->vbuf);
	mode.set();
	mode.clear();
  	mode.setWindow(0, 128);
	if (!gGame.ShowingMinimap() || pMinimap) {
		mFlags.view = VIEW_IDLE;
		mView.idle.Init();
	} else {
		pMinimap = this;
		mFlags.view = VIEW_MINIMAP;
		mView.minimap.Init();
	}
	gGame.NeedsSync();

}

void ViewSlot::HideSprites() {
	ViewMode mode = Graphics();
	for(unsigned i=0; i<8; ++i) {
		mode.hideSprite(i);
	}
}

void ViewSlot::SetView(unsigned viewId, bool doFlush, unsigned rid) {
	mFlags.view = viewId;
	ViewMode gfx = Graphics();
	if (GetCube()->vbuf.peekb(offsetof(_SYSVideoRAM, mode)) != _SYS_VM_BG0_SPR_BG1) {
		gfx.setWindow(0,128);
		gfx.init();
	} else {
		gfx.BG0_setPanning(Vec2(0,0));
	}
	if (pInventory == this && viewId != VIEW_INVENTORY) { 
		pInventory = FindIdleView();
		if (pInventory) { pInventory->SetView(VIEW_INVENTORY, doFlush); }
	}
	if (pMinimap == this && viewId != VIEW_MINIMAP) { 
		pMinimap = FindIdleView();
		if (pMinimap) { 
			pMinimap->SetView(VIEW_MINIMAP, doFlush); 
		} else if (pInventory) {
			pInventory->SetView(VIEW_MINIMAP, doFlush);
			pInventory = 0;
		} else {
			pMinimap = 0; 
		}
	}
	switch(viewId) {
		case VIEW_IDLE:
			mView.idle.Init();
			break;
		case VIEW_ROOM:
			mView.room.Init(rid);
			break;
		case VIEW_INVENTORY:
			pInventory = this;
			mView.inventory.Init();
			break;
		case VIEW_MINIMAP:
			pMinimap = this;
			mView.minimap.Init();
			break;
	}
	if (doFlush) {
		#if GFX_ARTIFACT_WORKAROUNDS
			gGame.Paint(true);
			GetCube()->vbuf.touch();
		#endif
			gGame.Paint(true);
	}
}

void ViewSlot::Restore(bool doFlush) {
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
	case VIEW_MINIMAP:
		mView.minimap.Restore();
		break;
	}
	if (doFlush) {
		#if GFX_ARTIFACT_WORKAROUNDS
			gGame.Paint(true);
			GetCube()->vbuf.touch();
		#endif
			gGame.Paint(true);
	}
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
	case VIEW_MINIMAP:
		mView.minimap.Update(dt);
		break;
	}
}
  
bool ViewSlot::ShowLocation(Vec2 loc, bool force, bool doFlush) {
	if (!gGame.GetMap()->Contains(loc)) {
		if (IsShowingRoom()) {
			HideLocation(doFlush);
		}
	} else {
		unsigned rid = gGame.GetMap()->GetRoomId(loc);
		if (force || !IsShowingRoom() || mView.room.GetRoom()->Id() != rid) {
			SetView(VIEW_ROOM, doFlush, rid);
			return true;
		}
	}
	return false;
}

ViewSlot* ViewSlot::FindIdleView() {
	for (ViewSlot *p=gGame.ViewBegin(); p!=gGame.ViewEnd(); ++p) {
		if (p->ViewType() == VIEW_IDLE) { return p; }
	}
	return 0;
}

bool ViewSlot::HideLocation(bool doFlush) {
	if (IsShowingRoom()) {
		if (gGame.ShowingMinimap() && !pMinimap) {
			SetView(VIEW_MINIMAP, doFlush);
		} else if (gGame.GetState()->HasAnyItems() && !pInventory) {
			SetView(VIEW_INVENTORY, doFlush);
		} else {
			SetView(VIEW_IDLE, doFlush);
		}
		return true;
	}
	return false;
}

void ViewSlot::RefreshInventory(bool doFlush) {
  if (mFlags.view == VIEW_INVENTORY) {
  	if (!gGame.GetState()->HasAnyItems()) {
  		SetView(VIEW_IDLE, doFlush);
  	} else {
  		mView.inventory.OnInventoryChanged();
  	}
  } else if (mFlags.view == VIEW_IDLE && gGame.GetState()->HasAnyItems() && !pInventory) { 
	SetView(VIEW_INVENTORY, doFlush);
  }
}

ViewSlot* ViewSlot::VirtualNeighborAt(Cube::Side side) const {
  Cube::ID neighbor = GetCube()->virtualNeighborAt(side);
  return neighbor == CUBE_ID_UNDEFINED ? 0 : gGame.ViewAt(neighbor-CUBE_ID_BASE);
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
