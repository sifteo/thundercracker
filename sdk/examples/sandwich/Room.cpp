#include "Room.h"
#include "Game.h"

unsigned Room::Id() const {
  return (int)(this - pGame->GetMap()->GetRoom(0));
}

Vec2 Room::LocalCenter(unsigned subdiv) const { 
  if (subdiv) {
    ASSERT(mUserdataType == USERDATA_SUBDIV);
    if (mInnerType == SUBDIV_DIAG_POS || mInnerType == SUBDIV_DIAG_NEG) {
      const DiagonalSubdivisionData* p = SubdivAsDiagonal();
      return Vec2(p->altCenterX, p->altCenterY); 
    } else if (mInnerType == SUBDIV_BRDG_HOR || mInnerType == SUBDIV_BRDG_VER) {
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
  mUserdataType = USERDATA_SUBDIV;
  mInnerType = diag->positiveSlope ? SUBDIV_DIAG_POS : SUBDIV_DIAG_NEG;
  mUserdata = diag;
}

void Room::SetBridgeSubdivision(const BridgeSubdivisionData* bridge) {
  mUserdataType = USERDATA_SUBDIV;
  mInnerType = bridge->isHorizontal ? SUBDIV_BRDG_HOR : SUBDIV_BRDG_VER;
  mUserdata = bridge;
}

void Room::Clear() { 
  mUserdataType = 0;
  mInnerType = 0;
  mUserdata = 0;
  mDoor = 0;
  mOverlayIndex = 0xffff;
}