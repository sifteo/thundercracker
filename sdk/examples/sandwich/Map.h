#pragma once
#include "Base.h"
#include "Content.h"

#define ROOM_CAPACITY (16*16)
#define PATH_CAPACITY (32)

struct MapRoom {
	int itemId;

  int RoomId() const;
  Vec2 Location() const;
  RoomData* Data() const;

  inline Vec2 Position() const { return 128 * Location(); }
  inline Vec2 LocalCenter() const { return Vec2(Data()->centerx, Data()->centery); }
  inline Vec2 Center() const { return Position() + 16 * LocalCenter(); }

  void SetItem(int itemId);
  uint8_t GetPortal(Cube::Side side);
  void SetPortal(Cube::Side side, uint8_t pid);

  uint8_t GetTile(Vec2 position);
  void SetTile(Vec2 position, uint8_t tid);
  void OpenDoor(/*Cube::Side side*/);
};

struct MapPath {
  uint8_t moves[PATH_CAPACITY];
  uint8_t *pFirstMove;
  inline int Length() const { return (moves + PATH_CAPACITY) - pFirstMove; }
  inline const uint8_t* End() const { return moves + PATH_CAPACITY; }
};

class Map {
private:
  MapData* mData;
  MapRoom mRooms[ROOM_CAPACITY];

public:
  Map();
  
  // Map Data Getters
  MapData* Data() const { return mData; }
  inline const uint8_t* GetPortalX(int x, int y) {
      // note that the pitch here is one greater than the width because there's
      // an extra wall on the right side of the last tile in each row
      ASSERT(0 <= x && x <= mData->width);
      ASSERT(0 <= y && y < mData->height);
      return mData->xportals + (y * (mData->width+1) + x);
  }

  inline const uint8_t* GetPortalY(int x, int y) {
      // Like GetPortalX except we're in column-major order
      ASSERT(0 <= x && x < mData->width);
      ASSERT(0 <= y && y <= mData->height);
      return mData->yportals + (x * (mData->height+1) + y);
  }
  
  inline const RoomData* GetRoomData(uint8_t roomId) const {
      ASSERT(roomId < mData->width * mData->height);
      return mData->rooms + roomId;
  }

  inline const RoomData* GetRoomData(Vec2 location) const {
      return GetRoomData(GetRoomId(location));
  }

  inline uint8_t GetTileId(Vec2 location, Vec2 tile) const {
      // this value indexes into tileset.frames
      ASSERT(0 <= location.x && location.x < mData->width);
      ASSERT(0 <= location.y && location.y < mData->height);
      ASSERT(0 <= tile.x && tile.x < 8);
      ASSERT(0 <= tile.y && tile.y < 8);
      return mData->rooms[location.y * mData->width + location.x].tiles[tile.y * 8 + tile.x];
  }

  inline bool IsTileOpen(Vec2 location, Vec2 tile) {
      ASSERT(0 <= location.x && location.x < mData->width);
      ASSERT(0 <= location.y && location.y < mData->height);
      ASSERT(0 <= tile.x && tile.x < 8);
      ASSERT(0 <= tile.y && tile.y < 8);
      return ( mData->rooms[location.y * mData->width + location.x].collisionMaskRows[tile.y] & (1<<tile.x) ) == 0;
  }

  inline uint8_t GetRoomId(Vec2 location) const {
      ASSERT(0 <= location.x && location.x < mData->width);
      ASSERT(0 <= location.y && location.y < mData->height);
      return location.x + location.y * mData->width;
  }

  inline Vec2 GetLocation(uint8_t roomId) const {
      ASSERT(roomId < mData->width * mData->height);
      return Vec2(
          roomId % mData->width,
          roomId / mData->width
      );
  }

  bool IsShowing(const MapData& map) const { return mData == &map; }
  bool CanTraverse(Vec2 loc, Cube::Side direction) const;
  void SetData(const MapData& map);
  
  MapRoom* GetRoom(int roomId) const { return (MapRoom*)mRooms + roomId; }
  MapRoom* GetRoom(Vec2 loc) const { return (MapRoom*)mRooms + (loc.x + mData->width * loc.y); }

  bool FindPath(Vec2 originLocation, Cube::Side direction, MapPath* outPath);
  bool IsVertexWalkable(Vec2 globalVertex);
};
