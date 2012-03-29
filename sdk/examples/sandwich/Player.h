#pragma once

#include "Map.h"
#include "ViewSlot.h"
class RoomView;
class Room;

#define WALK_SPEED 3
#define PLAYER_STATUS_IDLE 0
#define PLAYER_STATUS_WALKING 1

class Player {
private:
  int mStatus;
  BroadLocation mCurrent;
  BroadLocation mTarget;
  Int2 mPosition;
  uint8_t mDir;
  uint8_t mAnimFrame;
  const ItemData* mEquipment;
  float mAnimTime;

public:
  void Init(Cube* pPrimary);
  int AnimFrame();

  Room* GetRoom() const;

  inline BroadLocation* Current() { return &mCurrent; }
  inline BroadLocation* Target() { return &mTarget; }
  inline RoomView* CurrentView() { return mCurrent.view; }
  inline RoomView* TargetView() { return mTarget.view; }
  inline Room* CurrentRoom() { return mCurrent.view->GetRoom(); }
  inline Room* TargetRoom() { return mTarget.view->GetRoom(); }
  inline ViewSlot* View() const { return mTarget.view==0?mCurrent.view->Parent():mTarget.view->Parent(); }
  inline Cube::Side Direction() { return mDir; }
  inline bool TestCollision(Sokoblock* block) const { return (mPosition - block->Position()).len2() < (48*48); }
  inline Int2 Position() const { return mPosition; }
  inline Int2 Location() const { return View()->IsShowingRoom() ? View()->GetRoomView()->Location() : mPosition/128; }
  inline int Status() const { return mStatus; }
  inline const ItemData* Equipment() const { return mEquipment; }
  inline bool CanCrossLava() const { return mEquipment && gItemTypeData[mEquipment->itemId].triggerType == ITEM_TRIGGER_BOOT; }

  void ConsumeEquipment();

  void SetStatus(int status);
  inline void SetDirection(Cube::Side dir) { mDir = dir; }
  inline void SetPosition(Int2 position) { mPosition = position; }
  inline void SetEquipment(const ItemData *equipId) { mEquipment = equipId; }

  void ClearTarget();
  void AdvanceToTarget();

  void Move(int dx, int dy);
  inline void Move(Int2 delta) { Move(delta.x, delta.y); }
  void Update(float dt);
};