#include "Room.h"
#include "Game.h"

unsigned Room::Id() const {
  return (int)(this - pGame->GetMap()->GetRoom(0));
}

Vec2 Room::LocalCenter(unsigned subdiv) const { 
  if (subdiv) {
    if (mSubdivType == SUBDIV_DIAG_POS || mSubdivType == SUBDIV_DIAG_NEG) {
      const DiagonalSubdivisionData* p = SubdivAsDiagonal();
      return Vec2(p->altCenterX, p->altCenterY); 
    } else if (mSubdivType == SUBDIV_BRDG_HOR || mSubdivType == SUBDIV_BRDG_VER) {
      const BridgeSubdivisionData* p = SubdivAsBridge();
      return Vec2(p->altCenterX, p->altCenterY);
    }
  }
  return Vec2(Data()->centerX, Data()->centerY); 
}

Vec2 Room::Location() const {
  return pGame->GetMap()->GetLocation(Id());
}

const RoomData* Room::Data() const {
  return pGame->GetMap()->GetRoomData(Id());
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

const uint8_t* Room::OverlayBegin() const {
  return pGame->GetMap()->Data()->rle_overlay + mOverlayIndex;
}

void Room::SetDiagonalSubdivision(const DiagonalSubdivisionData* diag) {
  mSubdivType = diag->positiveSlope ? SUBDIV_DIAG_POS : SUBDIV_DIAG_NEG;
  mSubdiv = (const void*) diag;
}

void Room::SetBridgeSubdivision(const BridgeSubdivisionData* bridge) {
  mSubdivType = bridge->isHorizontal ? SUBDIV_BRDG_HOR : SUBDIV_BRDG_VER;
  mSubdiv = (const void*) bridge;
}

void Room::Clear() { 
  mTriggerType = TRIGGER_UNDEFINED;
  mTrigger = 0; 
  mDoor = 0;
  mOverlayIndex = 0xffff;
  mSubdivType = SUBDIV_NONE;
  mSubdiv = 0;
}