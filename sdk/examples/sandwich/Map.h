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
  MapRoom* GetRoom(int x, int y) const { return (MapRoom*)mRooms + x + mData->width * y; }
};
