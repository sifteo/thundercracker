#include "DrawingHelpers.h"
#include "Game.h"

void WaitForSeconds(float dt) {
  SystemTime deadline = SystemTime::now() + dt;
  do { gGame.DoPaint(); } while(deadline.inFuture());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const int8_t kHoverTable[HOVER_COUNT] = {
  0, 0, 1, 2, 2, 3, 3, 3, 
  4, 3, 3, 3, 2, 2, 1, 0, 
  -1, -1, -2, -3, -3, -4, 
  -4, -4, -4, -4, -4, -4, 
  -3, -3, -2, -1
};


#define BFF_FRAME_COUNT 4

namespace BffDir {
  enum ID { E, SE, S, SW, W, NW, N, NE };
}

static const Int2 sBffTable[] = {
  vec(1, 0),
  vec(1, 1),
  vec(0, 1),
  vec(-1, 1),
  vec(-1, 0),
  vec(-1, -1),
  vec(0, -1),
  vec(1, -1),
};

void ButterflyFriend::Randomize() {
  using namespace BffDir;
  dir = (uint8_t) gRandom.randrange(8);
  switch(dir) {
    case S:
      pos.x = 64 + (uint8_t) gRandom.randrange(128);
      pos.y = (uint8_t) gRandom.randrange(64);
      break;
    case SE:
      pos.x = (uint8_t) gRandom.randrange(64);
      pos.y = (uint8_t) gRandom.randrange(64);
      break;
    case E:
      pos.x = (uint8_t) gRandom.randrange(64);
      pos.y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case NE:
      pos.x = (uint8_t) gRandom.randrange(64);
      pos.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case N:
      pos.x = 64 + (uint8_t) gRandom.randrange(128);
      pos.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case NW:
      pos.x = 192 + (uint8_t) gRandom.randrange(64);
      pos.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case W:
      pos.x = 192 + (uint8_t) gRandom.randrange(64);
      pos.y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case SW:
      pos.x = 192 + (uint8_t) gRandom.randrange(128);
      pos.y = (uint8_t) gRandom.randrange(64);
      break;
  }
}

void ButterflyFriend::Update() {
    // butterfly stuff
    pos += UByte2(sBffTable[dir]);
    // hack - assumes butterflies and items are not rendered on same cube
    frame = (frame + 1) % (BFF_FRAME_COUNT * 3);
    using namespace BffDir;
    switch(dir) {
      case S:
        if (pos.y > 196) { Randomize(); }
        break;
      case SW:
        if (pos.x < 60 || pos.y > 196) { Randomize(); }
        break;
      case W:
        if (pos.x < 60) { Randomize(); }
        break;
      case NW:
        if (pos.x < 60 || pos.y < 60) { Randomize(); }
        break;
      case N:
      if (pos.y < 60) { Randomize(); }
        break;
      case NE:
      if (pos.x > 196 || pos.y < 60) { Randomize(); }
        break;
      case E:
      if (pos.x > 196) { Randomize(); }
        break;
      case SE:
      if (pos.x > 196 || pos.y > 196) { Randomize(); }
        break;
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DrawRoom(Viewport& gfx, int roomId) {
  auto& map = gGame.GetMap().Data();
	const uint8_t *pTile = map.roomTiles[roomId].tiles;
	const FlatAssetImage& tileset = *map.tileset;
	Int2 p;

	for(p.y=0; p.y<16; p.y+=2)
	for(p.x=0; p.x<16; p.x+=2) {
		// inline and optimize this function?
    gfx.Canvas().bg0.image(p, tileset, *(pTile++));
	}
}

void DrawRoomOverlay(Viewport& gfx, unsigned tid, const uint8_t *pRle) {
  // this method's a little annoyingly complex because metatiles are 2x2, so I need to replot
  // each row twice, in a sense, in order to get them in the correct bg1 mask order :P
  auto& map = gGame.GetMap().Data();
  const FlatAssetImage& img = *map.overlay;
  BG1Mask mask;
  mask.clear();
  unsigned location = 0;
  unsigned prevRow = 0;
  unsigned tileCount = 0;
  unsigned prevTileCount = 0;
  unsigned prevTid = tid;
  const uint8_t* prevRle = pRle;
  while(tid < 64) {
    // compute the current row
    unsigned row = tid >> 3;
    if (row > prevRow) {
      while (tileCount > prevTileCount) {
        // plot the "bottom half" of the metatiles from this row
        if (*prevRle == 0xff) {
          // skip ahead a run of transparent tiles
          prevTid += prevRle[1];
          prevRle+=2;
        } else {
          // plot the "bottom two" tiles of the metatile
          Int2 p = vec(prevTid%8, row)<<1;
          mask.plot(p+vec(0,1));
          mask.plot(p+vec(1,1));
          gfx.Canvas().bg1.plot(location++, img.tile(gfx.GetID(), vec(0,1), *prevRle));
          gfx.Canvas().bg1.plot(location++, img.tile(gfx.GetID(), vec(1,1), *prevRle));
          prevTileCount++;
          prevTid++;
          prevRle++;
        }
      }
      // sync up
      prevRow = row;
      prevTid = tid;
      prevRle = pRle;
    }
    if (*pRle == 0xff) {
      // skip ahead a run of transparent tiles
      tid += pRle[1];
      pRle+=2;
    } else {
      // plot the "top two" actual tiles of the metatile
      Int2 p = vec(tid%8, row)<<1;
      mask.plot(p);
      mask.plot(p+vec(1,0));
      gfx.Canvas().bg1.plot(location++, img.tile(gfx.GetID(), vec(0,0), *pRle));
      gfx.Canvas().bg1.plot(location++, img.tile(gfx.GetID(), vec(1,0), *pRle));
      tileCount++;
      tid++;
      pRle++;
    }
  }
  gfx.Canvas().bg1.setMask(mask, false);
}

void DrawOffsetMap(Viewport& gfx, Int2 pos) {
  // TODO: Refactor to use Forthcoming BG0 Scroller in SDK
  const MapData& map = gGame.GetMap().Data();
	const int xmax = 128 * (map.width-1);
	const int ymax = 128 * (map.height-1);
	if (pos.x < 0) { pos.x = 0; } else if (pos.x > xmax) { pos.x = xmax; }
	if (pos.y < 0) { pos.y = 0; } else if (pos.y > ymax) { pos.y = ymax; }
	Int2 loc = vec(pos.x>>7, pos.y>>7);
	Int2 pan = vec(pos.x - (loc.x << 7), pos.y - (loc.y << 7));
	gfx.Canvas().bg0.setPanning(pan);
	Int2 start_tile = vec(pan.x>>4, pan.y>>4);
	Int2 t;
	/*
	for(t.x=0; t.x<18; ++t.x)
	for(t.y=0; t.y<18; ++t.y) {
		gfx->bg0.image(t, Black);
	}
	*/
	// top-left room
	for(t.y=start_tile.y; t.y<8; ++t.y)
	for(t.x=start_tile.x; t.x<8; ++t.x) {
		gfx.Canvas().bg0.image(
			vec(t.x<<1, t.y<<1),
			*map.tileset,
			map.roomTiles[loc.x + loc.y * map.width].tiles[t.x + (t.y<<3)]
		);
	}
	
	if (pan.x) {
		// top-right room
		for(t.y=start_tile.y; t.y<8; ++t.y)
		for(t.x=0; t.x<=start_tile.x; ++t.x) {
			gfx.Canvas().bg0.image(
				vec((8 + t.x)%9<<1, t.y<<1),
				*map.tileset,
				map.roomTiles[(loc.x+1) + loc.y * map.width].tiles[t.x + (t.y<<3)]
			);
		}

		if (pan.y) {
			// bottom-right room
			for(t.y=0; t.y<=start_tile.y; ++t.y)
			for(t.x=0; t.x<=start_tile.x; ++t.x) {
				gfx.Canvas().bg0.image(
					vec((8 + t.x)%9<<1, (8 + t.y)%9<<1),
					*map.tileset,
					map.roomTiles[(loc.x+1) + (loc.y+1) * map.width].tiles[t.x + (t.y<<3)]
				);
			}
		}

	}
	
	if (pan.y) {
		// bottom-left room
		for(t.y=0; t.y<=start_tile.y; ++t.y)
		for(t.x=start_tile.x; t.x<8; ++t.x) {
			gfx.Canvas().bg0.image(
				vec(t.x<<1, (8 + t.y)%9<<1),
				*map.tileset,
				map.roomTiles[loc.x + (loc.y+1) * map.width].tiles[t.x + (t.y<<3)]
			);
		}
	}
		
}



