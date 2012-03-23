#include "Map.h"
#include "Game.h"

void Map::Init() {
  SetData(gMapData[gQuestData->mapId]);
}

void Map::SetData(const MapData& map) { 
  if (mData != &map) {
    mData = &map; 
    // triggers
    RefreshTriggers();
    // sokoblocks
    mBlockCount = 0;
    if (map.sokoblocks) {
      for (const SokoblockData* p=map.sokoblocks; p->x; ++p) {
        mBlock[mBlockCount].Init(*p);
        mBlockCount++;
      }
      ASSERT(mBlockCount <= BLOCK_CAPACITY);
    }
  }
}

static inline bool AtEnd(const TriggerData& trig) { return trig.eventType == 0xff; }

void Map::RefreshTriggers() {
  const Room* pEnd = mRooms + (mData->width*mData->height);
  for(Room* p=mRooms; p!=pEnd; ++p) { p->Clear(); }

  // find active triggers
  if (mData->items) {
    for(const ItemData* p = mData->items; !AtEnd(p->trigger); ++p) {
      if (gGame.GetState()->IsActive(p->trigger)) {
        ASSERT(!mRooms[p->trigger.room].HasUserdata());
        mRooms[p->trigger.room].SetTrigger(TRIGGER_ITEM, &p->trigger);
      }
    }
  }

  if (mData->gates) {
    for(const GatewayData* p = mData->gates; !AtEnd(p->trigger); ++p) {
      if (gGame.GetState()->IsActive(p->trigger)) {
        ASSERT(!mRooms[p->trigger.room].HasUserdata());
        mRooms[p->trigger.room].SetTrigger(TRIGGER_GATEWAY, &p->trigger);
      }
    }
  }
  
  if (mData->npcs) {
    for(const NpcData* p = mData->npcs; !AtEnd(p->trigger); ++p) {
      if (gGame.GetState()->IsActive(p->trigger)) {
        ASSERT(!mRooms[p->trigger.room].HasUserdata());
        mRooms[p->trigger.room].SetTrigger(TRIGGER_NPC, &p->trigger);
      }
    }
  }
  
  if (mData->trapdoors) {
    for(const TrapdoorData* p = mData->trapdoors; p->roomId!=p->respawnRoomId; ++p) {
      ASSERT(!mRooms[p->roomId].HasUserdata());
      mRooms[p->roomId].SetTrapdoor(p);
    }
  }
  
  if (mData->diagonalSubdivisions) {
    for(const DiagonalSubdivisionData* p = mData->diagonalSubdivisions; p->roomId; ++p) {
      ASSERT(!mRooms[p->roomId].HasUserdata());
      mRooms[p->roomId].SetDiagonalSubdivision(p);
    }
  }
  
  if (mData->bridgeSubdivisions) {
    for(const BridgeSubdivisionData* p = mData->bridgeSubdivisions; p->roomId; ++p) {
      ASSERT(!mRooms[p->roomId].HasUserdata());
      mRooms[p->roomId].SetBridgeSubdivision(p);
    }
  }
  
  if (mData->doors) {
    for(const DoorData* p = mData->doors; p->roomId; ++p) {
      mRooms[p->roomId].SetDoor(p);
    }
  }
  
  // walk the overlay RLE to find room breaks
  if (mData->rle_overlay) {
    const unsigned tend = mData->width * mData->height * 64; // 64 tiles per room
    unsigned tid=0;
    int prevRoomId = -1;
    const uint8_t *p = mData->rle_overlay;
    while (tid < tend) {
      if (*p == 0xff) {
        tid += p[1];
        p+=2;
      } else {
        const int roomId = (tid >> 6);
        if (roomId > prevRoomId) {
          prevRoomId = roomId;
          mRooms[roomId].SetOverlay(p - mData->rle_overlay, tid - (roomId<<6));
        }
        p++;
        tid++;
      }
    }
  }  
}

bool Map::IsVertexWalkable(Int2 vertex) {
  // convert global vertex into location / local vertex pair
  Int2 loc = Vec2(vertex.x>>3, vertex.y>>3);
  if (vertex.x == 0 || loc.x < 0 || loc.x >= mData->width || loc.y < 0 || loc.y >= mData->height) {
    return false; // out of bounds
  }
  vertex -= 8 * loc;
  return IsTileOpen(loc, vertex) && (
    vertex.x == 0 ? // are we between rooms?
      IsTileOpen(Vec2(loc.x-1, loc.y), Vec2(7, vertex.y)) : 
      IsTileOpen(loc, Vec2(vertex.x-1, vertex.y))
  );

}
