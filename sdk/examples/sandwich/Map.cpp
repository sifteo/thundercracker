#include "Map.h"
#include "Game.h"


//-------
// MAP ROOM STUFF

int MapRoom::RoomId() {
  return (int)(this - gGame.map.GetRoom(0));
}

Vec2 MapRoom::Location() {
  int id = RoomId();
  return Vec2(id % gGame.map.Data()->width, id / gGame.map.Data()->width);
}

RoomData* MapRoom::Data() {
  return gGame.map.Data()->GetRoomData(RoomId());
}

void MapRoom::SetItem(int iid) {
  if (itemId != iid) {
    itemId = iid;
    ItemData* id = gGame.map.Data()->FindItemData(RoomId());
    if (id) { id->itemId = itemId; }
  }
}

uint8_t MapRoom::GetPortal(Cube::Side side) {
  Vec2 p = Location();
  switch(side) {
    case SIDE_TOP: return gGame.map.Data()->GetPortalY(p.x, p.y);
    case SIDE_LEFT: return gGame.map.Data()->GetPortalX(p.x, p.y);
    case SIDE_BOTTOM: return gGame.map.Data()->GetPortalY(p.x, p.y+1);
    case SIDE_RIGHT: return gGame.map.Data()->GetPortalX(p.x+1, p.y);
  }
  return 0;
}

void MapRoom::SetPortal(Cube::Side side, uint8_t pid) {
  Vec2 p = Location();
  switch(side) {
    case SIDE_TOP: gGame.map.Data()->SetPortalY(p.x, p.y, pid); break;
    case SIDE_LEFT: gGame.map.Data()->SetPortalX(p.x, p.y, pid); break;
    case SIDE_BOTTOM: gGame.map.Data()->SetPortalY(p.x, p.y+1, pid); break;
    case SIDE_RIGHT: gGame.map.Data()->SetPortalX(p.x+1, p.y, pid); break;
  }
}

uint8_t MapRoom::GetTile(Vec2 position) {
  return Data()->tiles[position.x + 8 * position.y];
}

void MapRoom::SetTile(Vec2 position, uint8_t tid) {
 Data()->tiles[position.x + 8 * position.y] = tid; 
}

void MapRoom::OpenDoor(Cube::Side side) {
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
        Vec2(p.x+x, p.y+y), 
        GetTile(Vec2(p.x+x, p.y+y))+2
    );
    }
  }
}

void MapRoom::ClearTrigger() {
  if (callback) {
    callback = 0;
    TriggerData* t = gGame.map.Data()->FindTriggerData(RoomId());
    if (t) { t->callback = 0; }
  }
}

//-------
// MAP STUFF

Map::Map() {
  SetData(forest_data);
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

