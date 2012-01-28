#pragma once
#include "Base.h"
#include "Content.h"

class Room {
private:
	const TriggerData* mTrigger;
  const DoorData* mDoor;
  uint16_t mOverlayIndex;
  uint8_t mOverlayTile;
  uint8_t mTriggerType; // 4 bits unused

  void _Asserts() {
    STATIC_ASSERT((1<<8) >= TRIGGER_TYPE_COUNT ); // did we give mTriggerType enough bits?
  }


public:

  // basic getters
  int RoomId() const;
  Vec2 Location() const;
  const RoomData* Data() const;

  // telem getters
  inline Vec2 Position() const { return 128 * Location(); }
  inline Vec2 LocalCenter() const { return Vec2(Data()->centerx, Data()->centery); }
  inline Vec2 Center() const { return Position() + 16 * LocalCenter(); }
  //uint8_t GetTile(Vec2 position);

  // trigger getters
  const TriggerData* Trigger() const { return mTrigger; }
  int TriggerType() const { return mTriggerType; }
  bool HasTrigger() const { return mTrigger != 0; }
  bool HasGateway() const { return mTriggerType == TRIGGER_GATEWAY; }
  bool HasItem() const { return mTriggerType == TRIGGER_ITEM; }
  bool HasNPC() const { return mTriggerType == TRIGGER_NPC; }
  const GatewayData* TriggerAsGate() { ASSERT(mTriggerType == TRIGGER_GATEWAY); return (const GatewayData*) mTrigger; }
  const ItemData* TriggerAsItem() { ASSERT(mTriggerType == TRIGGER_ITEM); return (const ItemData*) mTrigger; }
  const NpcData* TriggerAsNPC() { ASSERT(mTriggerType == TRIGGER_NPC); return (const NpcData*) mTrigger; }

  // door getters
  bool HasDoor() const { return mDoor != 0; }
  bool HasOpenDoor() const;
  bool HasClosedDoor() const;

  bool HasOverlay() const { return mOverlayIndex != 0xffff; }
  const uint8_t* OverlayBegin() const;
  unsigned OverlayTile() const { return mOverlayTile; }

  // methods
  void SetTrigger(int type, const TriggerData* p) { mTriggerType = type; mTrigger = p; }
  void SetDoor(const DoorData* p) { mDoor = p; }
  void SetOverlay(uint16_t rleIndex, uint8_t firstTile) { mOverlayIndex = rleIndex; mOverlayTile = firstTile; }
  void Clear();
  void ClearTrigger() { mTriggerType = TRIGGER_UNDEFINED; mTrigger = 0; }
  bool OpenDoor();
};
