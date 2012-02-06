#pragma once
#include "Common.h"
#include "Content.h"

class Room {
private:
	const TriggerData* mTrigger;
  const DoorData* mDoor;
  const void* mSubdiv;
  uint16_t mOverlayIndex;
  uint8_t mOverlayTile;
  uint8_t mTriggerType : 4;
  uint8_t mSubdivType : 4;


  void _Asserts() {
    STATIC_ASSERT((1<<8) >= TRIGGER_TYPE_COUNT ); // did we give mTriggerType enough bits?
  }


public:

  // basic getters
  unsigned Id() const;
  Vec2 Location() const;
  const RoomData* Data() const;

  // telem getters
  inline Vec2 Position() const { return 128 * Location(); }
  Vec2 LocalCenter(unsigned subdiv) const;
  inline Vec2 Center(unsigned subdiv) const { return Position() + 16 * LocalCenter(subdiv); }
  //uint8_t GetTile(Vec2 position);

  // triggers
  void SetTrigger(int type, const TriggerData* p) { mTriggerType = type; mTrigger = p; }
  void ClearTrigger() { mTriggerType = TRIGGER_UNDEFINED; mTrigger = 0; }
  const TriggerData* Trigger() const { return mTrigger; }
  int TriggerType() const { return mTriggerType; }
  bool HasTrigger() const { return mTrigger != 0; }
  bool HasGateway() const { return mTriggerType == TRIGGER_GATEWAY; }
  bool HasItem() const { return mTriggerType == TRIGGER_ITEM; }
  bool HasNPC() const { return mTriggerType == TRIGGER_NPC; }
  const GatewayData* TriggerAsGate() { ASSERT(mTriggerType == TRIGGER_GATEWAY); return (const GatewayData*) mTrigger; }
  const ItemData* TriggerAsItem() { ASSERT(mTriggerType == TRIGGER_ITEM); return (const ItemData*) mTrigger; }
  const NpcData* TriggerAsNPC() { ASSERT(mTriggerType == TRIGGER_NPC); return (const NpcData*) mTrigger; }

  // subdivs
  bool IsSubdivided() const { return mSubdivType != SUBDIV_NONE; }
  bool IsBridge() const { return mSubdivType == SUBDIV_BRDG_VER || mSubdivType == SUBDIV_BRDG_HOR; }
  int SubdivType() const { return mSubdivType; }
  const DiagonalSubdivisionData* SubdivAsDiagonal() const { ASSERT(mSubdivType == SUBDIV_DIAG_POS || mSubdivType == SUBDIV_DIAG_NEG);  return (const DiagonalSubdivisionData*)mSubdiv; }
  const BridgeSubdivisionData* SubdivAsBridge() const { ASSERT(mSubdivType == SUBDIV_BRDG_VER || mSubdivType == SUBDIV_BRDG_HOR); return (const BridgeSubdivisionData*)mSubdiv; }
  void SetDiagonalSubdivision(const DiagonalSubdivisionData* diag);
  void SetBridgeSubdivision(const BridgeSubdivisionData* bridge);

  // general getters
  bool HasDoor() const { return mDoor != 0; }
  bool HasOpenDoor() const;
  bool HasClosedDoor() const;
  bool HasOverlay() const { return mOverlayIndex != 0xffff; }
  const uint8_t* OverlayBegin() const;
  unsigned OverlayTile() const { return mOverlayTile; }

  // general methods
  void SetDoor(const DoorData* p) { mDoor = p; }
  void SetOverlay(uint16_t rleIndex, uint8_t firstTile) { mOverlayIndex = rleIndex; mOverlayTile = firstTile; }
  void Clear();
  bool OpenDoor();
};
