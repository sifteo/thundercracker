#include "Map.h"

Map::Map() {
  SetData(dungeon_data);
}

inline static bool PortalOpen(uint8_t pid) { 
    return pid == PORTAL_DOOR || pid == PORTAL_OPEN;
}

void Map::SetData(const MapData& map) { 
  if (mData != (MapData*)&map) {
    mData = (MapData*)&map; 
    MapRoom *proom = mRooms;
    for(int y=0; y<map.height; ++y) {
      for(int x=0; x<map.width; ++x) {
        proom->callback = 0;
        proom->itemId = 0;
        ++proom;
      }
    }
    for(TriggerData* p = mData->triggers; p && p->room>=0; ++p) {
      (mRooms + p->room)->callback = p->callback;
    }
    for(ItemData* p = mData->items; p && p->room>=0; ++p) {
      (mRooms + p->room)->itemId = p->itemId;
    }    
  }
}

bool Map::CanTraverse(Vec2 tile, Cube::Side direction) const {
    switch(direction) {
      case SIDE_TOP:
        return tile.y > 0 && PortalOpen( mData->GetPortalY(tile.x, tile.y) );
      case SIDE_LEFT:
        return tile.x > 0 && PortalOpen( mData->GetPortalX(tile.x, tile.y) );
      case SIDE_BOTTOM:
        return tile.y < mData->height-1 && PortalOpen( mData->GetPortalY(tile.x, tile.y+1) );
      case SIDE_RIGHT:
        return tile.x < mData->width-1 && PortalOpen( mData->GetPortalX(tile.x+1, tile.y) );
  }
  return false;
}

void Map::SetRoomItem(Vec2 room, int itemId) {
  MapRoom* p = GetRoom(room);
  if (p->itemId != itemId) {
    p->itemId = itemId;
    mData->GetItemData(room)->itemId = itemId;
  }
}