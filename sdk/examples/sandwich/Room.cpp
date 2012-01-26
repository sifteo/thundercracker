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

bool Room::HasOpenDoor() const {
  return HasDoor() && !pGame->state.IsActive(pGame->map.Data()->doorQuestId, mDoor->flagId);
}

bool Room::HasClosedDoor() const {
  return HasDoor() && pGame->state.IsActive(pGame->map.Data()->doorQuestId, mDoor->flagId);
}

bool Room::OpenDoor() {
  ASSERT(HasDoor());
  return pGame->state.Flag(pGame->map.Data()->doorQuestId, mDoor->flagId);
}

void Room::Clear() { 
  mTriggerType = TRIGGER_UNDEFINED;
  mTrigger = 0; 
  mDoor = 0;
}