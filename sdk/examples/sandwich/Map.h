#pragma once
#include "Base.h"
#include "MapData.h"
#include "gen_mapdata.h"

struct MapRoom {
	trigger_func callback;
	int itemId;
};

#define ROOM_CAPACITY (16*16)

class Map {
private:
  MapData* mData;
  MapRoom mRooms[ROOM_CAPACITY];

public:
  Map();
  MapData* Data() const { return mData; }
  bool IsShowing(const MapData& map) const { return mData == &map; }
  bool CanTraverse(Vec2 tile, Cube::Side direction) const;
  void SetData(const MapData& map);
  MapRoom* GetRoom(Vec2 room) const { return (MapRoom*)mRooms + room.x + mData->width * room.y; }
  void SetRoomItem(Vec2 room, int itemId);

  uint8_t GetPortal(Vec2 location, Cube::Side side);
  void SetPortal(Vec2 location, Cube::Side side, uint8_t pid);

  uint8_t GetTile(Vec2 location, Vec2 position);
  void SetTile(Vec2 location, Vec2 position, uint8_t tid);
  void OpenDoor(Vec2 location, Cube::Side side);
  
  void ClearTrigger(Vec2 location);
};
