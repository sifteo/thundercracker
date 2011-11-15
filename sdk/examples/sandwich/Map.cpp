#include "Map.h"

Map::Map() {
  SetData(dungeon_data);
}

inline static bool PortalOpen(uint8_t pid) { 
    return pid != PORTAL_WALL;
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
    ItemData* id = mData->FindItemData(mData->GetRoomId(room));
    if (id) { id->itemId = itemId; }
  }
}

uint8_t Map::GetPortal(Vec2 p, Cube::Side side) {
  switch(side) {
    case SIDE_TOP: return mData->GetPortalY(p.x, p.y);
    case SIDE_LEFT: return mData->GetPortalX(p.x, p.y);
    case SIDE_BOTTOM: return mData->GetPortalY(p.x, p.y+1);
    case SIDE_RIGHT: return mData->GetPortalX(p.x+1, p.y);
  }
  return 0;
}

void Map::SetPortal(Vec2 p, Cube::Side side, uint8_t pid) {
  switch(side) {
    case SIDE_TOP: mData->SetPortalY(p.x, p.y, pid); break;
    case SIDE_LEFT: mData->SetPortalX(p.x, p.y, pid); break;
    case SIDE_BOTTOM: mData->SetPortalY(p.x, p.y+1, pid); break;
    case SIDE_RIGHT: mData->SetPortalX(p.x+1, p.y, pid); break;
  }
}

uint8_t Map::GetTile(Vec2 location, Vec2 position) {
  return mData->GetTileId(location, position.x, position.y);
}

void Map::SetTile(Vec2 location, Vec2 position, uint8_t tid) {
  mData->SetTileId(location, position.x, position.y, tid);
}

void Map::OpenDoor(Vec2 location, Cube::Side side) {
  Vec2 p = Vec2(0,0);
  switch(side) {
    case SIDE_TOP: p = Vec2(3,0); break;
    case SIDE_LEFT: p = Vec2(0, 3); break;
    case SIDE_BOTTOM: p = Vec2(3, 6); break;
    case SIDE_RIGHT: p = Vec2(6, 3); break;
  }
  for(int x=0; x<2; ++x) {
    for(int y=0; y<2; ++y) {
      SetTile(
        location, 
        Vec2(p.x+x, p.y+y), 
        GetTile(location, Vec2(p.x+x, p.y+y))+2
    );
    }
  }
  
}

void Map::ClearTrigger(Vec2 location) {
  MapRoom *p = GetRoom(location);
  if (p->callback) {
    p->callback = 0;
    TriggerData* t = mData->FindTriggerData(mData->GetRoomId(location));
    if (t) { t->callback = 0; }
  }
}

