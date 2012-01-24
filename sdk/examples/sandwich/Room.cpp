#include "Room.h"
#include "Game.h"

int Room::RoomId() const {
  return (int)(this - pGame->map.GetRoom(0));
}

Vec2 Room::Location() const {
  int id = RoomId();
  return Vec2(id % pGame->map.Data()->width, id / pGame->map.Data()->width);
}

const RoomData* Room::Data() const {
  return pGame->map.GetRoomData(RoomId());
}

uint8_t Room::GetPortal(Cube::Side side) {
  Vec2 p = Location();
  switch(side) {
    case SIDE_TOP: return pGame->map.GetPortalY(p.x, p.y-1);
    case SIDE_LEFT: return pGame->map.GetPortalX(p.x-1, p.y);
    case SIDE_BOTTOM: return pGame->map.GetPortalY(p.x, p.y);
    case SIDE_RIGHT: return pGame->map.GetPortalX(p.x, p.y);
  }
  return 0;
}

uint8_t Room::GetTile(Vec2 position) {
  return Data()->tiles[position.x + 8 * position.y];
}

void Room::OpenDoor() {
  LOG(("OpenDoor TODO\n"));
  /*
  for(int row=0; row<3; ++row) {
    for(int col=3; col<=4; ++col) {
      SetTile(
        Vec2(col, row), GetTile(Vec2(col, row))+2
      );
    }
  }
  */
}