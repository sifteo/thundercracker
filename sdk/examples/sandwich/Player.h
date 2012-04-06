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

  BroadLocation* Current() { return &mCurrent; }
  BroadLocation* Target() { return &mTarget; }
  RoomView* CurrentView() { return mCurrent.view; }
  RoomView* TargetView() { return mTarget.view; }
  Room* CurrentRoom() { return mCurrent.view->GetRoom(); }
  Room* TargetRoom() { return mTarget.view->GetRoom(); }
  ViewSlot* View() const { return mTarget.view==0?mCurrent.view->Parent():mTarget.view->Parent(); }
  Cube::Side Direction() { return mDir; }
  bool TestCollision(Sokoblock* block) const { return (mPosition - block->Position()).len2() < (48*48); }
  Int2 Position() const { return mPosition; }
  Int2 Location() const { return View()->ShowingRoom() ? View()->GetRoomView()->Location() : mPosition/128; }
  int Status() const { return mStatus; }
  const ItemData* Equipment() const { return mEquipment; }
  bool CanCrossLava() const { return mEquipment && gItemTypeData[mEquipment->itemId].triggerType == ITEM_TRIGGER_BOOT; }

  void ConsumeEquipment();

  void SetStatus(int status);
  void SetDirection(Cube::Side dir) { mDir = dir; }
  void SetPosition(Int2 position) { mPosition = position; }
  void SetEquipment(const ItemData *equipId) { mEquipment = equipId; }

  void ClearTarget();
  void AdvanceToTarget();

  void Move(int dx, int dy);
  void Move(Int2 delta) { Move(delta.x, delta.y); }
  void Update();
};