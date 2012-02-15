#pragma once
#include "Room.h"

#define ROOM_CAPACITY (81)
#define PATH_CAPACITY (32)
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
  bool PopStep(BroadLocation newRoot, BroadLocation* outNext);
  void Cancel();
};

struct NarrowPath {
  uint8_t moves[PATH_CAPACITY];
  uint8_t *pFirstMove;
  inline int Length() const { return (moves + PATH_CAPACITY) - pFirstMove; }
  inline const uint8_t* End() const { return moves + PATH_CAPACITY; }
};

//-----------------------------------------------------------------------------
// A "View" of the currently loaded map, accessible via pGame->GetMap()
//-----------------------------------------------------------------------------

class Map {
private:
  const MapData* mData;
  Room mRooms[ROOM_CAPACITY];

public:
  void Init();

  void SetData(const MapData& map);
  
  Room* GetRoom(int roomId) const { return (Room*)mRooms + roomId; }
  Room* GetRoom(Vec2 loc) const { return (Room*)mRooms + (loc.x + mData->width * loc.y); }

  bool CanTraverse(BroadLocation loc, Cube::Side side);
  bool GetBroadLocationNeighbor(BroadLocation loc, Cube::Side side, BroadLocation* outNeighbor);
  bool IsVertexWalkable(Vec2 globalVertex);
  bool FindBroadPath(BroadPath* outPath);
  bool FindNarrowPath(BroadLocation loc, Cube::Side direction, NarrowPath* outPath);


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

  inline const RoomData* GetRoomData(Vec2 location) const {
    return GetRoomData(GetRoomId(location));
  }

  inline uint8_t GetTileId(unsigned roomId, Vec2 tile) const {
    ASSERT(0 <= tile.x && tile.x < 8);
    ASSERT(0 <= tile.y && tile.y < 8);
    return mData->rooms[roomId].tiles[(tile.y<<3) + tile.x];
  }

  inline bool IsTileOpen(Vec2 location, Vec2 tile) const {
    ASSERT(0 <= location.x && location.x < mData->width);
    ASSERT(0 <= location.y && location.y < mData->height);
    ASSERT(0 <= tile.x && tile.x < 8);
    ASSERT(0 <= tile.y && tile.y < 8);
    return ( mData->rooms[location.y * mData->width + location.x].collisionMaskRows[tile.y] & (1<<tile.x) ) == 0;
  }

  inline uint8_t GetRoomId(Vec2 location) const {
    ASSERT(Contains(location));
    return location.x + location.y * mData->width;
  }

  inline Vec2 GetLocation(uint8_t roomId) const {
    return Vec2(roomId % mData->width, roomId / mData->width);
  }

  inline bool Contains(Vec2 loc) const {
    return loc.x >= 0 && loc.y >= 0 && loc.x < mData->width && loc.y < mData->height;
  }

};
