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

void Viewport::Init() {
	mFlags.view = VIEW_IDLE;
	mFlags.currTouch = 0;
	mFlags.prevTouch = 0;
	mFlags.hasOverlay = 0;
	EnqueueHackyTouches();
	RestoreCanonicalVideo();
	mCanvas.bg0.erase(BlackTile);
	mView.idle.Init();
}

void Viewport::RestoreCanonicalVideo() {
	if (mCanvas.mode() == BG0_SPR_BG1) {
	  	mCanvas.bg0.setPanning(vec(0,0));
		for(unsigned i=0; i<8; ++i) { mCanvas.sprites[i].hide(); }
		if (mFlags.hasOverlay) {
			mCanvas.bg1.erase(); 
			mFlags.hasOverlay = 0;
		}
	} else {
		mCanvas.initMode(BG0_SPR_BG1);
	}
	EnqueueHackyTouches();
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
	mFlags.view = view;
	RestoreCanonicalVideo();
	//EvictSecondaryView(view);
	if (view == VIEW_ROOM) {
		mView.room.Init(roomId);
	} else {
		mView.edge.Init(roomId, side);
	}
	return true;
}

void Viewport::SetSecondaryView(unsigned viewId) {
	mFlags.view = viewId;
	RestoreCanonicalVideo();
	//EvictSecondaryView(viewId);
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
	RestoreCanonicalVideo();
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

void Viewport::UpdateTouch() {
	mFlags.currTouch = GetID().isTouching();
}

void Viewport::Update() {
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
	mFlags.prevTouch = mFlags.currTouch;
	if (mFlags.touchHack) {
		mCanvas.touch();
		mFlags.touchHack--;
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
	auto& map = gGame.GetMap().Data();
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
	return SetLocationView(gGame.GetMap().GetRoomId(loc), side, force);
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
		//if (gGame.ShowingMinimap() && !pMinimap) {
		//	SetSecondaryView(VIEW_MINIMAP);
		//} else if (gGame.GetState().HasAnyItems() && !pInventory) {
		//	SetSecondaryView(VIEW_INVENTORY);
		//} else {
			SetSecondaryView(VIEW_IDLE);
		//}
		return true;
	}
	return false;
}

void Viewport::RefreshInventory() {
  if (mFlags.view == VIEW_INVENTORY) {
  	if (!gGame.GetState().HasAnyItems()) {
  		SetSecondaryView(VIEW_IDLE);
  	} else {
  		mView.inventory.OnInventoryChanged();
  	}
  } else if (mFlags.view == VIEW_IDLE && gGame.GetState().HasAnyItems() && !pInventory) { 
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


//-----------------------------------------------------------------------------
// DRAWING HELPERS
//-----------------------------------------------------------------------------

void Viewport::DrawRoom(unsigned roomId) {
  	auto& map = gGame.GetMap().Data();
	const FlatAssetImage& tileset = *map.tileset;
	Int2 p;
  	if (map.tileType == TILE_TYPE_8) {
		const uint8_t *pTile = reinterpret_cast<const uint8_t*>(map.roomTiles) + (roomId<<6);
		for(p.y=0; p.y<16; p.y+=2)
		for(p.x=0; p.x<16; p.x+=2) {
			// inline and optimize this function?
	    	mCanvas.bg0.image(p, tileset, *(pTile++));
		}
  	} else {
		const uint16_t *pTile = reinterpret_cast<const uint16_t*>(map.roomTiles) + (roomId<<6);
		for(p.y=0; p.y<16; p.y+=2)
		for(p.x=0; p.x<16; p.x+=2) {
			// inline and optimize this function?
	    	mCanvas.bg0.image(p, tileset, *(pTile++));
		}
  	}
}

void Viewport::DrawRoomOverlay(unsigned tid, const uint8_t *rle) {
  // this method's a little annoyingly complex because metatiles are 2x2, so I need to replot
  // each row twice, in a sense, in order to get them in the correct bg1 mask order :P
  auto& map = gGame.GetMap().Data();
  const FlatAssetImage& img = *map.overlay;
  BG1Mask mask;
  mask.clear();
  unsigned location = 0;
  unsigned tileCount = 0;
  unsigned prevTileCount = 0;
  unsigned prevTid = tid;
  const uint8_t* prevRle = rle;
  unsigned row = tid >> 3;
  unsigned prevRow = row;
  while(tid < 64) {
    // compute the current row
    if (*rle == 0xff) {
      // skip ahead a run of transparent tiles
      tid += rle[1];
      rle+=2;
    } else {
      // plot the "top two" actual tiles of the metatile
      Int2 p = vec(tid%8, row)<<1;
      mask.plot(p);
      mask.plot(p+vec(1,0));
      mCanvas.bg1.plot(location++, img.tile(GetID(), vec(0,0), *rle));
      mCanvas.bg1.plot(location++, img.tile(GetID(), vec(1,0), *rle));
      tileCount++;
      tid++;
      rle++;
    }
	row = tid >> 3;  
	// does the next row need to catch up?  
    if (row > prevRow || tid >= 64) {
      while (prevTid < tid) {
        // plot the "bottom half" of the metatiles from this row
        if (*prevRle == 0xff) {
          // skip ahead a run of transparent tiles
          prevTid += prevRle[1];
          prevRle+=2;
        } else {
          // plot the "bottom two" tiles of the metatile
          Int2 p = vec(prevTid%8, prevRow)<<1;
          mask.plot(p+vec(0,1));
          mask.plot(p+vec(1,1));
          mCanvas.bg1.plot(location++, img.tile(GetID(), vec(0,1), *prevRle));
          mCanvas.bg1.plot(location++, img.tile(GetID(), vec(1,1), *prevRle));
          prevTileCount++;
          prevTid++;
          prevRle++;
        }
      }
      // sync up
      prevRow = row;
      prevRle = rle;
    }    
  }
  mCanvas.bg1.setMask(mask, false);
}

void Viewport::DrawOffsetMap(Int2 pos) {
  	// TODO: Refactor to use Forthcoming BG0 Scroller in SDK
	// This isn't great code anyway because of the use of convenience GetTileId(),
	// which internally has a conditional branch that could be moved outside the whole
	// method body.
  	auto& map = gGame.GetMap();
  	auto& data = map.Data();
  	auto& tileset = *map.Data().tileset;
	const int xmax = 128 * (data.width-1);
	const int ymax = 128 * (data.height-1);
	if (pos.x < 0) { pos.x = 0; } else if (pos.x > xmax) { pos.x = xmax; }
	if (pos.y < 0) { pos.y = 0; } else if (pos.y > ymax) { pos.y = ymax; }
	Int2 loc = vec(pos.x>>7, pos.y>>7);
	Int2 pan = vec(pos.x - (loc.x << 7), pos.y - (loc.y << 7));
	mCanvas.bg0.setPanning(pan);
	Int2 start_tile = vec(pan.x>>4, pan.y>>4);
	Int2 t;
	// top-left room
	for(t.y=start_tile.y; t.y<8; ++t.y)
	for(t.x=start_tile.x; t.x<8; ++t.x) {
		mCanvas.bg0.image(
			vec(t.x<<1, t.y<<1),
			tileset,
			map.GetTileId(loc.x + loc.y * data.width, t.x + (t.y<<3))
		);
	}
	
	if (pan.x) {
		// top-right room
		for(t.y=start_tile.y; t.y<8; ++t.y)
		for(t.x=0; t.x<=start_tile.x; ++t.x) {
			mCanvas.bg0.image(
				vec((8 + t.x)%9<<1, t.y<<1),
				tileset,
				map.GetTileId((loc.x+1) + loc.y * data.width, t.x + (t.y<<3))
			);
		}

		if (pan.y) {
			// bottom-right room
			for(t.y=0; t.y<=start_tile.y; ++t.y)
			for(t.x=0; t.x<=start_tile.x; ++t.x) {
				mCanvas.bg0.image(
					vec((8 + t.x)%9<<1, (8 + t.y)%9<<1),
					tileset,
					map.GetTileId((loc.x+1) + (loc.y+1) * data.width, t.x + (t.y<<3))
				);
			}
		}

	}
	if (pan.y) {
		// bottom-left room
		for(t.y=0; t.y<=start_tile.y; ++t.y)
		for(t.x=start_tile.x; t.x<8; ++t.x) {
			mCanvas.bg0.image(
				vec(t.x<<1, (8 + t.y)%9<<1),
				tileset,
				map.GetTileId(loc.x + (loc.y+1) * data.width, t.x + (t.y<<3))
			);
		}
	}
		
}



