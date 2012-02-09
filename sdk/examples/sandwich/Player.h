#pragma once

#include "Map.h"
class RoomView;
class Room;

#define WALK_SPEED 3
#define PLAYER_STATUS_IDLE 0
#define PLAYER_STATUS_WALKING 1

class Player {
private:
  CORO_PARAMS
  int mStatus;
  BroadLocation mCurrent;
  BroadLocation mTarget;
  Vec2 mPosition;
  int mDir;
  int mAnimFrame;
  float mAnimTime;

  // stately variables
  int mProgress;
  int mNextDir;
  bool mApproachingLockedDoor;
  float mTimeout;

  BroadPath mPath;
  NarrowPath mMoves;
  uint8_t* pNextMove;
    
public:
  void Init(Cube* pPrimary);
  int AnimFrame();
  const BroadLocation* Current() { return &mCurrent; }
  const BroadLocation* Target() { return &mTarget; }
  RoomView* View() { return mTarget.view==0?mCurrent.view:mTarget.view; }
  Cube::Side Direction() { return mDir; }
  Vec2 Position() const { return mPosition; }
  Vec2 Location() const { return mPosition/128; }
  Room* CurrentRoom() const;
  int Status() const { return mStatus; }

  void SetLocation(Vec2 position, Cube::Side direction);
  void SetPosition(Vec2 position) { mPosition = position; }

  void Move(int dx, int dy);
  void Update(float dt);
  void UpdateAnimation(float dt);
  void Reset();

private:
  void CheckForPassiveTrigger();
  void CheckForActiveTrigger();
};