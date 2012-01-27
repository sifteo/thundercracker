#include "Room.h"
#include "Game.h"

int Room::RoomId() const {
  return (int)(this - pGame->GetMap()->GetRoom(0));
}

Vec2 Room::Location() const {
  int id = RoomId();
  return Vec2(id % pGame->GetMap()->Data()->width, id / pGame->GetMap()->Data()->width);
}

const RoomData* Room::Data() const {
  return pGame->GetMap()->GetRoomData(RoomId());
}

bool Room::HasOpenDoor() const {
  return HasDoor() && !pGame->GetState()->IsActive(pGame->GetMap()->Data()->doorQuestId, mDoor->flagId);
}

bool Room::HasClosedDoor() const {
  return HasDoor() && pGame->GetState()->IsActive(pGame->GetMap()->Data()->doorQuestId, mDoor->flagId);
}

bool Room::OpenDoor() {
  ASSERT(HasDoor());
  return pGame->GetState()->Flag(pGame->GetMap()->Data()->doorQuestId, mDoor->flagId);
}

void Room::Clear() { 
  mTriggerType = TRIGGER_UNDEFINED;
  mTrigger = 0; 
  mDoor = 0;
}