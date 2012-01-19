#pragma once

#include "Map.h"
class GameView;
class Room;

#define WALK_SPEED 3
#define PLAYER_STATUS_IDLE 0
#define PLAYER_STATUS_WALKING 1

struct Path {
  Cube::Side steps[NUM_CUBES-1];
  Path();
  bool IsDefined() const { return *steps >= 0; }
  bool PopStep(GameView* newRoot);
  void Cancel();
};

class Player {
private:
  CORO_PARAMS
  int mStatus;
  GameView* pCurrent;
  GameView* pTarget;
  Vec2 mPosition;
  int mDir;
  int mKeyCount;
  int mItemMask;
  int mAnimFrame;
  float mAnimTime;

  // stately variables
  int mProgress;
  int mNextDir;
  bool mApproachingLockedDoor;
  float mTimeout;

  Path mPath;
  MapPath mMoves;
  uint8_t* pNextMove;
    
public:
  Player();
  
  int CurrentFrame();
  GameView* CurrentView() { return pCurrent; }
  GameView* TargetView() { return pTarget; }
  GameView* KeyView() { return pTarget==0?pCurrent:pTarget; }
  Cube::Side Direction() { return mDir; }
  Vec2 Position() const { return mPosition; }
  Vec2 Location() const { return mPosition/128; }
  Room* CurrentRoom() const;
  int Status() const { return mStatus; }

  void PickupItem(int itemId);
  bool HaveBasicKey() const { return mKeyCount > 0; }
  bool HasItem(int itemId) const { return (mItemMask & (1<<itemId)) != 0; }
  void DecrementBasicKeyCount();

  void SetLocation(Vec2 position, Cube::Side direction);
  void SetPosition(Vec2 position) { mPosition = position; }

  void Move(int dx, int dy);
  void Update(float dt);
  void UpdateAnimation(float dt);
  void Reset();
  
private:
  bool PathDetect();
  bool PathVisit(GameView* view, int depth);
};