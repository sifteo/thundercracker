#pragma once
#include "Room.h"

#define ROOM_CAPACITY (81)
#define PATH_CAPACITY (32)

struct MapPath {
  uint8_t moves[PATH_CAPACITY];
  uint8_t *pFirstMove;
  inline int Length() const { return (moves + PATH_CAPACITY) - pFirstMove; }
  inline const uint8_t* End() const { return moves + PATH_CAPACITY; }
};

class Map {
private:
  MapData* mData;
  Room mRooms[ROOM_CAPACITY];

public:

  void Init();

  // Map Data Getters

  const MapData* Data() const { return mData; }
  
  inline const bool GetPortalX(int x, int y) const {
      // note that the pitch here is one less than the width because 
      // we're only counting walls in between
      ASSERT(0 <= x && x < mData->width-1);
      ASSERT(0 <= y && y < mData->height);
      const int idx = (y * (mData->width-1) + x);
      return mData->xportals[idx/8] & (1<<(idx%8));
  }

  inline const bool GetPortalY(int x, int y) const {
      // Like GetPortalX except we're in column-major order
      ASSERT(0 <= x && x < mData->width);
      ASSERT(0 <= y && y < mData->height-1);
      const int idx = (x * (mData->height-1) + y);
      return mData->yportals[idx/8] & (1<<(idx%8));
  }
  
  inline const RoomData* GetRoomData(uint8_t roomId) const {
      ASSERT(roomId < mData->width * mData->height);
      return mData->rooms + roomId;
  }

  inline const RoomData* GetRoomData(Vec2 location) const {
      return GetRoomData(GetRoomId(location));
  }

  inline uint8_t GetTileId(unsigned roomId, Vec2 tile) const {
    ASSERT(roomId < (unsigned) mData->width * mData->height);
    ASSERT(0 <= tile.x && tile.x < 8);
    ASSERT(0 <= tile.y && tile.y < 8);
  return mData->rooms[roomId].tiles[(tile.y<<3) + tile.x];
  }

  inline uint8_t GetTileId(Vec2 location, Vec2 tile) const {
    // this value indexes into tileset.frames
    ASSERT(0 <= location.x && location.x < mData->width);
    ASSERT(0 <= location.y && location.y < mData->height);
    ASSERT(0 <= tile.x && tile.x < 8);
    ASSERT(0 <= tile.y && tile.y < 8);
    return mData->rooms[location.y * mData->width + location.x].tiles[(tile.y<<3) + tile.x];
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
    ASSERT(roomId < mData->width * mData->height);
    return Vec2(roomId % mData->width, roomId / mData->width);
  }

  inline bool Contains(Vec2 loc) const {
    return loc.x >= 0 && loc.y >= 0 && loc.x < mData->width && loc.y < mData->height;
  }

  bool IsShowing(const MapData& map) const { return mData == &map; }
  bool CanTraverse(Vec2 loc, Cube::Side direction) const;
  void SetData(const MapData& map);
  
  Room* GetRoom(int roomId) const { return (Room*)mRooms + roomId; }
  Room* GetRoom(Vec2 loc) const { return (Room*)mRooms + (loc.x + mData->width * loc.y); }

  bool FindPath(Vec2 originLocation, Cube::Side direction, MapPath* outPath);
  bool IsVertexWalkable(Vec2 globalVertex);
};
