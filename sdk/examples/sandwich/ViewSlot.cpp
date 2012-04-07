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

void ViewSlot::SanityCheckVram() {
	ViewMode gfx(GetCube()->vbuf);
	//if (GetCube()->vbuf.peekb(offsetof(_SYSVideoRAM, mode)) != _SYS_VM_BG0_SPR_BG1) {
	if (!gfx.isInMode()) {
		gfx.setWindow(0,128);
		gfx.init();
	}
	gfx.BG0_setPanning(vec(0,0));
}

void ViewSlot::EvictSecondaryView(unsigned viewId, bool doFlush) {
	if (pInventory == this && viewId != VIEW_INVENTORY) { 
		pInventory = FindIdleView();
		if (pInventory) { pInventory->SetSecondaryView(VIEW_INVENTORY, doFlush); }
	}
	if (pMinimap == this && viewId != VIEW_MINIMAP) { 
		pMinimap = FindIdleView();
		if (pMinimap) { 
			pMinimap->SetSecondaryView(VIEW_MINIMAP, doFlush); 
		} else if (pInventory) {
			pInventory->SetSecondaryView(VIEW_MINIMAP, doFlush);
			pInventory = 0;
		} else {
			pMinimap = 0; 
		}
	}
}

bool ViewSlot::SetLocationView(unsigned roomId, Cube::Side side, bool force, bool doFlush) {
	unsigned view = side == SIDE_UNDEFINED ? VIEW_ROOM : VIEW_EDGE;
	if (!force && mFlags.view == view) {
		if (view == VIEW_ROOM && mView.room.Id() == roomId) { return false; }
		if (view == VIEW_EDGE && mView.edge.Id() == roomId && mView.edge.Side() == side) { return false; }
	}
	SanityCheckVram();
	mFlags.view = view;
	EvictSecondaryView(view, doFlush);
	if (view == VIEW_ROOM) {
		mView.room.Init(roomId);
	} else {
		mView.edge.Init(roomId, side);
	}
	if (doFlush) {
		#if GFX_ARTIFACT_WORKAROUNDS
			gGame.Paint(true);
			GetCube()->vbuf.touch();
		#endif
			gGame.Paint(true);
	}
	return true;
}

void ViewSlot::SetSecondaryView(unsigned viewId, bool doFlush) {
	mFlags.view = viewId;
	SanityCheckVram();
	EvictSecondaryView(viewId, doFlush);
	switch(viewId) {
		case VIEW_IDLE:
			mView.idle.Init();
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
	case VIEW_EDGE:
		mView.edge.Restore();
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
	case VIEW_ROOM:
		mView.room.Update(dt);
		break;
	case VIEW_INVENTORY:
		mView.inventory.Update(dt);
		break;
	case VIEW_MINIMAP:
		mView.minimap.Update(dt);
		break;
	case VIEW_EDGE:
		mView.edge.Update(dt);
		break;
	}
}
  
bool ViewSlot::ShowLocation(Int2 loc, bool force, bool doFlush) {
	// possibilities: show room, show edge, show corner
	const MapData& map = *gGame.GetMap()->Data();
	Cube::Side side = SIDE_UNDEFINED;
	if (loc.x < -1) {
		HideLocation(doFlush);
		return false;
	} else if (loc.x == -1) {
		if (loc.y >= 0 && loc.y < map.height) {
			loc.x = 0;
			side = SIDE_LEFT;
		} else {
			HideLocation(doFlush);
			return false;
		}
	} else if (loc.x < map.width) {
		if (loc.y < -1) {
			HideLocation(doFlush);
			return false;
		} else if (loc.y == -1) {
			loc.y = 0;
			side = SIDE_TOP;
		} else if (loc.y < map.height) {
			// noop
		} else if (loc.y == map.height) {
			loc.y = map.height-1;
			side = SIDE_BOTTOM;
		} else {
			HideLocation(doFlush);
			return false;
		}
	} else if (loc.x == map.width) {
		if (loc.y >= 0 && loc.y < map.height) {
			loc.x = map.width-1;
			side = SIDE_RIGHT;
		} else {
			HideLocation(doFlush);
			return false;
		}
	} else {
		HideLocation(doFlush);
		return false;
	}
	return SetLocationView(gGame.GetMap()->GetRoomId(loc), side, force, doFlush);
}

ViewSlot* ViewSlot::FindIdleView() {
	for (ViewSlot *p=gGame.ViewBegin(); p!=gGame.ViewEnd(); ++p) {
		if (p->ViewType() == VIEW_IDLE) { return p; }
	}
	return 0;
}

bool ViewSlot::HideLocation(bool doFlush) {
	if (IsShowingLocation()) {
		if (gGame.ShowingMinimap() && !pMinimap) {
			SetSecondaryView(VIEW_MINIMAP, doFlush);
		} else if (gGame.GetState()->HasAnyItems() && !pInventory) {
			SetSecondaryView(VIEW_INVENTORY, doFlush);
		} else {
			SetSecondaryView(VIEW_IDLE, doFlush);
		}
		return true;
	}
	return false;
}

void ViewSlot::RefreshInventory(bool doFlush) {
  if (mFlags.view == VIEW_INVENTORY) {
  	if (!gGame.GetState()->HasAnyItems()) {
  		SetSecondaryView(VIEW_IDLE, doFlush);
  	} else {
  		mView.inventory.OnInventoryChanged();
  	}
  } else if (mFlags.view == VIEW_IDLE && gGame.GetState()->HasAnyItems() && !pInventory) { 
	SetSecondaryView(VIEW_INVENTORY, doFlush);
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
  Int2 accel = GetCube()->virtualAccel();
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
