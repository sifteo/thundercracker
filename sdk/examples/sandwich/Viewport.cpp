#include "Game.h"

Viewport* pInventory = 0;
Viewport* pMinimap = 0;

Viewport& Viewport::Iterator::operator*() {
	return gGame.ViewAt(currentId);
}

Viewport* Viewport::Iterator::operator->() {
	return &gGame.ViewAt(currentId);
}

Viewport::Iterator::operator Viewport*() {
	return &gGame.ViewAt(currentId);
}

bool Viewport::Touched() const {
  return !mFlags.prevTouch && GetCube().isTouching();
}

void Viewport::Init() {
	mFlags.view = VIEW_IDLE;
	mView.idle.Init();
}

void Viewport::HideSprites() {
	for(unsigned i=0; i<8; ++i) {
		mCanvas.sprites[i].hide();
	}
}

void Viewport::SanityCheckVram() {
	System::finish();
	if (mCanvas.mode() != BG0_SPR_BG1) {
		mCanvas.initMode(BG0_SPR_BG1);
		mCanvas.setWindow(0,128);
	}
	mCanvas.bg0.setPanning(vec(0,0));
}

void Viewport::EvictSecondaryView(unsigned viewId) {
	if (pInventory == this && viewId != VIEW_INVENTORY) { 
		pInventory = FindIdleView();
		if (pInventory) { pInventory->SetSecondaryView(VIEW_INVENTORY); }
	}
	if (pMinimap == this && viewId != VIEW_MINIMAP) { 
		pMinimap = FindIdleView();
		if (pMinimap) { 
			pMinimap->SetSecondaryView(VIEW_MINIMAP); 
		} else if (pInventory) {
			pInventory->SetSecondaryView(VIEW_MINIMAP);
			pInventory = 0;
		} else {
			pMinimap = 0; 
		}
	}
}

bool Viewport::SetLocationView(unsigned roomId, Side side, bool force) {
	unsigned view = side == NO_SIDE ? VIEW_ROOM : VIEW_EDGE;
	if (!force && mFlags.view == view) {
		if (view == VIEW_ROOM && mView.room.Id() == roomId) { return false; }
		if (view == VIEW_EDGE && mView.edge.Id() == roomId && mView.edge.GetSide() == side) { return false; }
	}
	SanityCheckVram();
	mFlags.view = view;
	EvictSecondaryView(view);
	if (view == VIEW_ROOM) {
		mView.room.Init(roomId);
	} else {
		mView.edge.Init(roomId, side);
	}
	return true;
}

void Viewport::SetSecondaryView(unsigned viewId) {
	mFlags.view = viewId;
	SanityCheckVram();
	EvictSecondaryView(viewId);
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
}

void Viewport::Restore() {
	mCanvas.setMode(BG0_SPR_BG1);
  	mCanvas.setWindow(0, 128);
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
  
bool Viewport::ShowLocation(Int2 loc, bool force) {
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
		HideLocation();
		return false;
	} else if (loc.x == -1) {
		if (loc.y >= 0 && loc.y < map.height) {
			loc.x = 0;
			side = LEFT;
		} else {
			HideLocation();
			return false;
		}
	} else if (loc.x < map.width) {
		if (loc.y < -1) {
			HideLocation();
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
			HideLocation();
			return false;
		}
	} else if (loc.x == map.width) {
		if (loc.y >= 0 && loc.y < map.height) {
			loc.x = map.width-1;
			side = RIGHT;
		} else {
			HideLocation();
			return false;
		}
	} else {
		HideLocation();
		return false;
	}
	return SetLocationView(gGame.GetMap()->GetRoomId(loc), side, force);
}

Viewport* Viewport::FindIdleView() {
	Viewport::Iterator p = gGame.ListViews();
	while(p.MoveNext()) {
		if (p->ViewType() == VIEW_IDLE) { return p; }
	}
	return 0;
}

bool Viewport::HideLocation() {
	if (ShowingLockedRoom()) {
		mView.room.ShowFrame();
		return false;
	}
	if (ShowingLocation()) {
		if (gGame.ShowingMinimap() && !pMinimap) {
			SetSecondaryView(VIEW_MINIMAP);
		} else if (gGame.GetState()->HasAnyItems() && !pInventory) {
			SetSecondaryView(VIEW_INVENTORY);
		} else {
			SetSecondaryView(VIEW_IDLE);
		}
		return true;
	}
	return false;
}

void Viewport::RefreshInventory() {
  if (mFlags.view == VIEW_INVENTORY) {
  	if (!gGame.GetState()->HasAnyItems()) {
  		SetSecondaryView(VIEW_IDLE);
  	} else {
  		mView.inventory.OnInventoryChanged();
  	}
  } else if (mFlags.view == VIEW_IDLE && gGame.GetState()->HasAnyItems() && !pInventory) { 
	SetSecondaryView(VIEW_INVENTORY);
  }
}

Viewport* Viewport::VirtualNeighborAt(Side side) const {
	Neighborhood hood = mCanvas.virtualNeighbors();
	CubeID neighbor = hood.neighborAt(side);
	return neighbor.isDefined() ? &gGame.ViewAt(neighbor-CUBE_ID_BASE) : 0;
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
  Int2 accel = mCanvas.virtualAccel().xy();
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
