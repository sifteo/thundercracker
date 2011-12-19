#pragma once
#include "Base.h"
#include "MapData.h"
#include "gen_mapdata.h"

#define ROOM_CAPACITY (16*16)
#define PATH_CAPACITY (32)

struct MapRoom {
	trigger_func callback;
	int itemId;

  int RoomId() const;
  Vec2 Location() const;
  RoomData* Data() const;

  inline Vec2 Position() const { return 128 * Location(); }
  inline Vec2 Center() const { return Position() + 16 * Data()->LocalCenter(); }

  void SetItem(int itemId);
  uint8_t GetPortal(Cube::Side side);
  void SetPortal(Cube::Side side, uint8_t pid);

  uint8_t GetTile(Vec2 position);
  void SetTile(Vec2 position, uint8_t tid);
  void OpenDoor(/*Cube::Side side*/);

  void ClearTrigger();
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
  MapData* Data() const { return mData; }
  bool IsShowing(const MapData& map) const { return mData == &map; }
  bool CanTraverse(Vec2 loc, Cube::Side direction) const;
  void SetData(const MapData& map);
  
  MapRoom* GetRoom(int roomId) const { return (MapRoom*)mRooms + roomId; }
  MapRoom* GetRoom(Vec2 loc) const { return (MapRoom*)mRooms + (loc.x + mData->width * loc.y); }

  bool FindPath(Vec2 originLocation, Cube::Side direction, MapPath* outPath);
  bool IsVertexWalkable(Vec2 globalVertex);

};
