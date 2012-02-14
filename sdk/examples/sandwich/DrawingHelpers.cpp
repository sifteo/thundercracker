#include "DrawingHelpers.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define BFF_FRAME_COUNT 4

namespace BffDir {
  enum ID { E, SE, S, SW, W, NW, N, NE };
}

static const Vec2 sBffTable[] = {
  Vec2(1, 0),
  Vec2(1, 1),
  Vec2(0, 1),
  Vec2(-1, 1),
  Vec2(-1, 0),
  Vec2(-1, -1),
  Vec2(0, -1),
  Vec2(1, -1),
};

void ButterflyFriend::Randomize() {
  using namespace BffDir;
  dir = (uint8_t) gRandom.randrange(8);
  switch(dir) {
    case S:
      x = 64 + (uint8_t) gRandom.randrange(128);
      y = (uint8_t) gRandom.randrange(64);
      break;
    case SE:
      x = (uint8_t) gRandom.randrange(64);
      y = (uint8_t) gRandom.randrange(64);
      break;
    case E:
      x = (uint8_t) gRandom.randrange(64);
      y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case NE:
      x = (uint8_t) gRandom.randrange(64);
      y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case N:
      x = 64 + (uint8_t) gRandom.randrange(128);
      y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case NW:
      x = 192 + (uint8_t) gRandom.randrange(64);
      y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case W:
      x = 192 + (uint8_t) gRandom.randrange(64);
      y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case SW:
      x = 192 + (uint8_t) gRandom.randrange(128);
      y = (uint8_t) gRandom.randrange(64);
      break;
  }
}

void ButterflyFriend::Update() {
    // butterfly stuff
    Vec2 delta = sBffTable[dir];
    x += (uint8_t) delta.x;
    y += (uint8_t) delta.y;
    // hack - assumes butterflies and items are not rendered on same cube
    frame = (frame + 1) % (BFF_FRAME_COUNT * 3);
    using namespace BffDir;
    switch(dir) {
      case S:
        if (y > 196) { Randomize(); }
        break;
      case SW:
        if (x < 60 || y > 196) { Randomize(); }
        break;
      case W:
        if (x < 60) { Randomize(); }
        break;
      case NW:
        if (x < 60 || y < 60) { Randomize(); }
        break;
      case N:
      if (y < 60) { Randomize(); }
        break;
      case NE:
      if (x > 196 || y < 60) { Randomize(); }
        break;
      case E:
      if (x > 196) { Randomize(); }
        break;
      case SE:
      if (x > 196 || y > 196) { Randomize(); }
        break;
    }	
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DrawRoom(ViewMode* gfx, const MapData* pMap, int roomId) {
	const uint8_t *pTile = pMap->rooms[roomId].tiles;
	Vec2 p;
	for(p.y=0; p.y<16; p.y+=2)
	for(p.x=0; p.x<16; p.x+=2) {
		// inline and optimize this function?
		gfx->BG0_drawAsset(p, *(pMap->tileset), *(pTile++));
	}
}

bool DrawOffsetMapFromTo(ViewMode* gfx, const MapData* pMap, Vec2 from, Vec2 to) {
	// inserted as a potential optimization point, 
	// because we only need to "replot" the delta tiles
	DrawOffsetMap(gfx, pMap, to);
	return true;
}

void DrawOffsetMap(ViewMode* gfx, const MapData* pMap, Vec2 pos) {
	const int xmax = 128 * (pMap->width-1);
	const int ymax = 128 * (pMap->height-1);
	if (pos.x < 0) { pos.x = 0; } else if (pos.x > xmax) { pos.x = xmax; }
	if (pos.y < 0) { pos.y = 0; } else if (pos.y > ymax) { pos.y = ymax; }
	Vec2 loc = Vec2(pos.x>>7, pos.y>>7);
	Vec2 pan = Vec2(pos.x - (loc.x << 7), pos.y - (loc.y << 7));
	gfx->BG0_setPanning(pan);
	Vec2 start_tile = Vec2(pan.x>>4, pan.y>>4);
	Vec2 t;
	/*
	for(t.x=0; t.x<18; ++t.x)
	for(t.y=0; t.y<18; ++t.y) {
		gfx->BG0_drawAsset(t, Black);
	}
	*/
	// top-left room
	for(t.y=start_tile.y; t.y<8; ++t.y)
	for(t.x=start_tile.x; t.x<8; ++t.x) {
		gfx->BG0_drawAsset(
			Vec2(t.x<<1, t.y<<1),
			*pMap->tileset,
			pMap->rooms[loc.x + loc.y * pMap->width].tiles[t.x + (t.y<<3)]
		);
	}
	
	if (pan.x) {
		// top-right room
		for(t.y=start_tile.y; t.y<8; ++t.y)
		for(t.x=0; t.x<=start_tile.x; ++t.x) {
			gfx->BG0_drawAsset(
				Vec2((8 + t.x)%9<<1, t.y<<1),
				*pMap->tileset,
				pMap->rooms[(loc.x+1) + loc.y * pMap->width].tiles[t.x + (t.y<<3)]
			);
		}

		if (pan.y) {
			// bottom-right room
			for(t.y=0; t.y<=start_tile.y; ++t.y)
			for(t.x=0; t.x<=start_tile.x; ++t.x) {
				gfx->BG0_drawAsset(
					Vec2((8 + t.x)%9<<1, (8 + t.y)%9<<1),
					*pMap->tileset,
					pMap->rooms[(loc.x+1) + (loc.y+1) * pMap->width].tiles[t.x + (t.y<<3)]
				);
			}
		}

	}
	
	if (pan.y) {
		// bottom-left room
		for(t.y=0; t.y<=start_tile.y; ++t.y)
		for(t.x=start_tile.x; t.x<8; ++t.x) {
			gfx->BG0_drawAsset(
				Vec2(t.x<<1, (8 + t.y)%9<<1),
				*pMap->tileset,
				pMap->rooms[loc.x + (loc.y+1) * pMap->width].tiles[t.x + (t.y<<3)]
			);
		}
	}
		
}
