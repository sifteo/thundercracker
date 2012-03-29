#pragma once
#include "Room.h"
#include "Sokoblock.h"

#define ROOM_CAPACITY   (81)
#define PATH_CAPACITY   (32)
#define BLOCK_CAPACITY  (8)
#define TILE_CAPACITY 

//-----------------------------------------------------------------------------
// DATA STRUCTURES FOR PATH - these are a bit messy as a side-effect of 
// their organic implementation; might be considered for cleanup in a refactor
// if this becomes a material problem.
//-----------------------------------------------------------------------------

class RoomView;

struct BroadLocation {
  RoomView *view; // uses views instead of rooms because it only applies to visible rooms (for now?)
  unsigned subdivision;
};

struct BroadPath {
  Cube::Side steps[2*NUM_CUBES]; // assuming each cube could be visited twice...
  BroadPath();
  bool IsDefined() const { return *steps >= 0; }
  bool DequeueStep(BroadLocation newRoot, BroadLocation* outNext);
  void Cancel();
};

struct NarrowPath {
  Cube::Side moves[PATH_CAPACITY];
  Cube::Side *pFirstMove;
  inline int Length() const { return (moves + PATH_CAPACITY) - pFirstMove; }
  inline const Cube::Side* Begin() { return pFirstMove; }
  inline const Cube::Side* End() { return moves + PATH_CAPACITY; }
};

//-----------------------------------------------------------------------------
// A "View" of the currently loaded map, accessible via pGame->GetMap()
//-----------------------------------------------------------------------------

class Map {
private:
  const MapData* mData;
  Room mRooms[ROOM_CAPACITY];

  // these non-room-based entities are naively allocated here.
  // we may need to create some other data structure to act as 
  // a lightweight generic pool allocator or something :P
  Sokoblock mBlock[BLOCK_CAPACITY];
  uint8_t mBlockCount;

public:
  void Init();

  void SetData(const MapData& map);
  void RefreshTriggers();

  Room* GetRoom(int roomId) const { return (Room*)mRooms + roomId; }
  Room* GetRoom(Int2 loc) const { return (Room*)mRooms + (loc.x + mData->width * loc.y); }

  bool CanTraverse(BroadLocation loc, Cube::Side side);
  bool GetBroadLocationNeighbor(BroadLocation loc, Cube::Side side, BroadLocation* outNeighbor);
  bool IsVertexWalkable(Int2 globalVertex);
  bool FindBroadPath(BroadPath* outPath, unsigned* outViewId);
  bool FindNarrowPath(BroadLocation loc, Cube::Side direction, NarrowPath* outPath);

  bool BlockCount() const { return mBlockCount; }
  Sokoblock* BlockBegin() { return mBlock; }
  Sokoblock* BlockEnd() { return mBlock + mBlockCount; }


  // Map Data Getters

  inline const MapData* Data() const { return mData; }
  
  inline const bool GetPortalX(int x, int y) const {
    // note that the pitch here is one less than the width because 
    // we're only counting walls in between
    ASSERT(0 <= x && x < mData->width-1);
    ASSERT(0 <= y && y < mData->height);
    const int idx = (y * (mData->width-1) + x);
    return mData->xportals[idx>>3] & (1<<(idx%8));
  }

  inline const bool GetPortalY(int x, int y) const {
    // Like GetPortalX except we're in column-major order
    ASSERT(0 <= x && x < mData->width);
    ASSERT(0 <= y && y < mData->height-1);
    const int idx = (x * (mData->height-1) + y);
    return mData->yportals[idx>>3] & (1<<(idx%8));
  }
  
  inline const RoomData* GetRoomData(int roomId) const {
    ASSERT(roomId < mData->width * mData->height);
    return mData->rooms + roomId;
  }

  inline const RoomData* GetRoomData(Int2 location) const {
    return GetRoomData(GetRoomId(location));
  }

  inline uint8_t GetTileId(unsigned roomId, Int2 tile) const {
    ASSERT(0 <= tile.x && tile.x < 8);
    ASSERT(0 <= tile.y && tile.y < 8);
    return mData->roomTiles[roomId].tiles[(tile.y<<3) + tile.x];
  }

  inline uint8_t GetGlobalTileId(Int2 tile) {
    const Int2 loc = tile >> 3;
    return GetTileId(loc.x + mData->width * loc.y, tile - (loc<<3));
  }

  inline bool IsTileOpen(Int2 location, Int2 tile) const {
    ASSERT(0 <= location.x && location.x < mData->width);
    ASSERT(0 <= location.y && location.y < mData->height);
    ASSERT(0 <= tile.x && tile.x < 8);
    ASSERT(0 <= tile.y && tile.y < 8);
    return ( mData->rooms[location.y * mData->width + location.x].collisionMaskRows[tile.y] & (1<<tile.x) ) == 0;
  }

  inline bool IsTileLava(uint8_t tid) {
    // Effectively O(1) since the length of lavaTiles is capped
    for(const uint8_t *p=mData->lavaTiles; p&&*p; ++p) { if (*p==tid) return true; }
    return false;
  }

  inline uint8_t GetRoomId(Int2 location) const {
    ASSERT(Contains(location));
    return location.x + location.y * mData->width;
  }

  inline Int2 GetLocation(uint8_t roomId) const {
    ASSERT(roomId < mData->width * mData->height);
    return Vec2(roomId % mData->width, roomId / mData->width);
  }

  inline bool Contains(Int2 loc) const {
    return loc.x >= 0 && loc.y >= 0 && loc.x < mData->width && loc.y < mData->height;
  }

};
