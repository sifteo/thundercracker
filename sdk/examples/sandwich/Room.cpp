#include "Room.h"
#include "Game.h"

unsigned Room::Id() const {
  return (int)(this - gGame.GetMap()->GetRoom(0));
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
  return gGame.GetMap()->GetLocation(Id());
}

const RoomData* Room::Data() const {
  return gGame.GetMap()->GetRoomData(Id());
}

bool Room::HasOpenDoor() const {
  return HasDoor() && !gGame.GetState()->IsActive(gGame.GetMap()->Data()->doorQuestId, mDoor->flagId);
}

bool Room::HasClosedDoor() const {
  return HasDoor() && gGame.GetState()->IsActive(gGame.GetMap()->Data()->doorQuestId, mDoor->flagId);
}

bool Room::OpenDoor() {
  ASSERT(HasDoor());
  return gGame.GetState()->Flag(gGame.GetMap()->Data()->doorQuestId, mDoor->flagId);
}

const uint8_t* Room::OverlayBegin() const {
  return gGame.GetMap()->Data()->rle_overlay + mOverlayIndex;
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

bool Room::IsShowingBlock(const Sokoblock* pBlock) {
  ASSERT(pBlock);
  const Vec2 blockTopLeft = pBlock->Position() - Vec2(32, 32);
  const Vec2 roomTopLeft = 128 * Location();
  const Vec2 delta = blockTopLeft - roomTopLeft;
  return delta.x > -64 && delta.x < 64 && delta.y > -64 && delta.y < 64;
}

unsigned Room::CountOpenTilesAlongSide(Cube::Side side) {
  ASSERT(0 <= side && side < 4);
  const Vec2 loc = Location();
  const Map& map = *gGame.GetMap();
  const RoomData& data = *Data();
  unsigned cnt = 0;
  switch(side) {
    case SIDE_TOP:
      if (loc.y > 0 && map.GetPortalY(loc.x, loc.y-1)) {
        cnt = 8-Intrinsic::POPCOUNT(data.collisionMaskRows[0]);
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
        cnt = 8-Intrinsic::POPCOUNT(~data.collisionMaskRows[7]);
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