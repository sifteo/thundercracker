#include "Game.h"

Viewport* pInventory = 0;
Viewport* pMinimap = 0;

Viewport& Viewport::Iterator::operator*() {
	return *gGame.ViewAt(currentId);
}

Viewport* Viewport::Iterator::operator->() {
	return gGame.ViewAt(currentId);
}

//Viewport* Viewport::Iterator::ptr() {
Viewport::Iterator::operator Viewport*() {
	return gGame.ViewAt(currentId);
}

bool Viewport::Touched() const {
  return !mFlags.prevTouch && GetCube().isTouching();
}

void Viewport::Init() {
	mBuffer.initMode(BG0_SPR_BG1);
  	mBuffer.setWindow(0, 128);
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

void Viewport::HideSprites() {
	for(unsigned i=0; i<8; ++i) {
		mBuffer.sprites[i].hide();
	}
}

void Viewport::SanityCheckVram() {
	if (mBuffer.mode() != BG0_SPR_BG1) {
		mBuffer.setWindow(0,128);
		mBuffer.initMode(BG0_SPR_BG1);
	}
	mBuffer.bg0.setPanning(vec(0,0));
}

void Viewport::EvictSecondaryView(unsigned viewId, bool doFlush) {
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

bool Viewport::SetLocationView(unsigned roomId, Side side, bool force, bool doFlush) {
	unsigned view = side == NO_SIDE ? VIEW_ROOM : VIEW_EDGE;
	if (!force && mFlags.view == view) {
		if (view == VIEW_ROOM && mView.room.Id() == roomId) { return false; }
		if (view == VIEW_EDGE && mView.edge.Id() == roomId && mView.edge.GetSide() == side) { return false; }
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
		gGame.DoPaint(true);
	}
	return true;
}

void Viewport::SetSecondaryView(unsigned viewId, bool doFlush) {
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
			gGame.DoPaint(true);
	}
}

void Viewport::Restore(bool doFlush) {
	mBuffer.setMode(BG0_SPR_BG1);
  	mBuffer.setWindow(0, 128);
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
		gGame.DoPaint(true);
	}
}

void Viewport::Update() {
	mFlags.prevTouch = GetCube().isTouching();
	switch(mFlags.view) {
	case VIEW_ROOM:
		mView.room.Update();
		break;
	case VIEW_INVENTORY:
		mView.inventory.Update();
		break;
	case VIEW_MINIMAP:
		mView.minimap.Update();
		break;
	case VIEW_EDGE:
		mView.edge.Update();
		break;
	}
}
  
bool Viewport::ShowLocation(Int2 loc, bool force, bool doFlush) {
	if (ShowingLockedRoom()) {
		if (loc == mView.room.Location()) {
			mView.room.Restore();
			return true;
		} else {
			mView.room.ShowFrame();
			return false;
		}
	}

	// possibilities: show room, show edge, show corner
	const MapData& map = *gGame.GetMap()->Data();
	Side side = NO_SIDE;
	if (loc.x < -1) {
		HideLocation(doFlush);
		return false;
	} else if (loc.x == -1) {
		if (loc.y >= 0 && loc.y < map.height) {
			loc.x = 0;
			side = LEFT;
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
			side = TOP;
		} else if (loc.y < map.height) {
			// noop
		} else if (loc.y == map.height) {
			loc.y = map.height-1;
			side = BOTTOM;
		} else {
			HideLocation(doFlush);
			return false;
		}
	} else if (loc.x == map.width) {
		if (loc.y >= 0 && loc.y < map.height) {
			loc.x = map.width-1;
			side = RIGHT;
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

Viewport* Viewport::FindIdleView() {
	Viewport::Iterator p = gGame.ListViews();
	while(p.MoveNext()) {
		if (p->ViewType() == VIEW_IDLE) { return p; }
	}
	return 0;
}

bool Viewport::HideLocation(bool doFlush) {
	if (ShowingLockedRoom()) {
		mView.room.ShowFrame();
		return false;
	}
	if (ShowingLocation()) {
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

void Viewport::RefreshInventory(bool doFlush) {
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

Viewport* Viewport::VirtualNeighborAt(Side side) const {
	Neighborhood hood = mBuffer.virtualNeighbors();
	CubeID neighbor = hood.neighborAt(side);
	return neighbor.isDefined() ? 0 : gGame.ViewAt(neighbor-CUBE_ID_BASE);
}

//----------------------------------------------------------------------
// ACCELEROMETER SHTUFF
//----------------------------------------------------------------------

#ifdef SIFTEO_SIMULATOR
#define TILT_THRESHOLD 50
#else
#define TILT_THRESHOLD 35
#endif

Side Viewport::VirtualTiltDirection() const {
  Int2 accel = mBuffer.virtualAccel().xy();
  if (accel.y < -TILT_THRESHOLD) {
    return TOP;
  } else if (accel.x < -TILT_THRESHOLD) {
    return LEFT;
  } else if (accel.y > TILT_THRESHOLD) {
    return BOTTOM;
  } else if (accel.x > TILT_THRESHOLD) {
    return RIGHT;
  } else {
    return NO_SIDE;
  }
}
