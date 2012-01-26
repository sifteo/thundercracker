#pragma once
#include "Base.h"
#include "Content.h"

class Room {
private:
  int mTriggerType;
	const TriggerData* mTrigger;
  const DoorData* mDoor;

public:

  // basic getters

  int RoomId() const;
  Vec2 Location() const;
  const RoomData* Data() const;

  // telem getters

  inline Vec2 Position() const { return 128 * Location(); }
  inline Vec2 LocalCenter() const { return Vec2(Data()->centerx, Data()->centery); }
  inline Vec2 Center() const { return Position() + 16 * LocalCenter(); }
  uint8_t GetPortal(Cube::Side side);
  uint8_t GetTile(Vec2 position);

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
  // methods
  
  void SetTrigger(int type, const TriggerData* p) { mTriggerType = type; mTrigger = p; }
  void SetDoor(const DoorData* p) { mDoor = p; }
  void Clear();
  void ClearTrigger() { mTriggerType = TRIGGER_UNDEFINED; mTrigger = 0; }

  bool OpenDoor();
};
