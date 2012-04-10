#include "Room.h"
#include "Game.h"

unsigned Room::Id() const {
  return (int)(this - gGame.GetMap()->GetRoom(0));
}

Int2 Room::LocalCenter(unsigned subdiv) const { 
  if (subdiv) {
    ASSERT(mPrimarySlotType == PRIMARY_SUBDIV);
    if (mPrimarySlotId == SUBDIV_DIAG_POS || mPrimarySlotId == SUBDIV_DIAG_NEG) {
      const DiagonalSubdivisionData* p = SubdivAsDiagonal();
      return vec(p->altCenterX, p->altCenterY); 
    } else if (mPrimarySlotId == SUBDIV_BRDG_HOR || mPrimarySlotId == SUBDIV_BRDG_VER) {
      const BridgeSubdivisionData* p = SubdivAsBridge();
      return vec(p->altCenterX, p->altCenterY);
    }
  }
  return vec(Data()->centerX, Data()->centerY); 
}

Int2 Room::Location() const {
  return gGame.GetMap()->GetLocation(Id());
}

const RoomData* Room::Data() const {
  return gGame.GetMap()->GetRoomData(Id());
}

bool Room::HasOpenDoor() const {
  return HasDoor() && !gGame.GetState()->IsActive(Door()->trigger);
}

bool Room::HasClosedDoor() const {
  return HasDoor() && gGame.GetState()->IsActive(Door()->trigger);
}

bool Room::OpenDoor() {
  ASSERT(HasDoor());
  return gGame.GetState()->FlagTrigger(Door()->trigger);
}

const uint8_t* Room::OverlayBegin() const {
  return gGame.GetMap()->Data()->rle_overlay + mOverlayIndex;
}

void Room::SetDiagonalSubdivision(const DiagonalSubdivisionData* diag) {
  ASSERT(!mPrimarySlot);
  mPrimarySlotType = PRIMARY_SUBDIV;
  mPrimarySlotId = diag->positiveSlope ? SUBDIV_DIAG_POS : SUBDIV_DIAG_NEG;
  mPrimarySlot = diag;
}

void Room::SetBridgeSubdivision(const BridgeSubdivisionData* bridge) {
  ASSERT(!mPrimarySlot);
  mPrimarySlotType = PRIMARY_SUBDIV;
  mPrimarySlotId = bridge->isHorizontal ? SUBDIV_BRDG_HOR : SUBDIV_BRDG_VER;
  mPrimarySlot = bridge;
}

void Room::Clear() { 
  mPrimarySlotType = 0;
  mPrimarySlotId = 0;
  mPrimarySlot = 0;
  mSecondarySlot = 0;
  mOverlayIndex = 0xffff;
  mBomb.canMask = 0;
  mBomb.didMask = 0;
}

bool Room::IsShowingBlock(const Sokoblock* pBlock) {
  ASSERT(pBlock);
  const Int2 blockTopLeft = pBlock->Position() - vec(32, 32);
  const Int2 roomTopLeft = 128 * Location();
  const Int2 delta = blockTopLeft - roomTopLeft;
  return delta.x > -64 && delta.x < 64 && delta.y > -64 && delta.y < 64;
}

unsigned Room::CountOpenTilesAlongSide(Cube::Side side) {
  ASSERT(0 <= side && side < 4);
  const Int2 loc = Location();
  const Map& map = *gGame.GetMap();
  const RoomData& data = *Data();
  unsigned cnt = 0;
  switch(side) {
    case SIDE_TOP:
      if (loc.y > 0 && map.GetPortalY(loc.x, loc.y-1)) {
        cnt = 8-__builtin_popcount(data.collisionMaskRows[0]);
      }
      break;
    case SIDE_LEFT:
      if (loc.x > 0 && map.GetPortalY(loc.x-1, loc.y)) {
        for(unsigned row=0; row<8; ++row) {
          cnt += (~data.collisionMaskRows[row]) & 0x01;
        }
      }
      break;
    case SIDE_BOTTOM:
      if (loc.y < map.Data()->height-1 && map.GetPortalY(loc.x, loc.y)) {
        cnt = 8-__builtin_popcount(~data.collisionMaskRows[7]);
      }
      break;
    case SIDE_RIGHT:
      if (loc.x < map.Data()->width-1 && map.GetPortalX(loc.x, loc.y)) {
        for(unsigned row=0; row<8; ++row) {
          cnt += (data.collisionMaskRows[row]) & 0x80;
        }
        cnt >>= 7;
      }
      break;
  }
  return cnt;
}