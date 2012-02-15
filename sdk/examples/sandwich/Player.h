#pragma once

#include "Map.h"
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
  Vec2 mPosition;
  int mDir;
  int mAnimFrame;
  float mAnimTime;
    
public:
  void Init(Cube* pPrimary);
  int AnimFrame();

  Room* GetRoom() const;

  inline BroadLocation* Current() { return &mCurrent; }
  inline BroadLocation* Target() { return &mTarget; }
  inline RoomView* CurrentView() { return mCurrent.view; }
  inline RoomView* TargetView() { return mTarget.view; }

  inline RoomView* View() { return mTarget.view==0?mCurrent.view:mTarget.view; }
  inline Cube::Side Direction() { return mDir; }
  inline Vec2 Position() const { return mPosition; }
  inline Vec2 Location() const { return mPosition/128; }
  inline int Status() const { return mStatus; }

  void SetStatus(int status);
  void SetDirection(Cube::Side dir) { mDir = dir; }
  void SetLocation(Vec2 position, Cube::Side direction);
  void OffsetPosition(Vec2 offset) { mPosition += offset; }
  void SetPosition(Vec2 position) { mPosition = position; }

  void ClearTarget();
  void AdvanceToTarget();

  void Move(int dx, int dy);
  void Update(float dt);
};