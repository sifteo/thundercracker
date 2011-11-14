#pragma once
#include "Base.h"
#include <sifteo.h>

#define ENEMY_PIXELS_PER_FRAME 1

class GameView;

class Enemy {
private:
  Vec2 mTile;
  Vec2 mStart;
  Vec2 mEnd;
  int mProgress;
  int mDistance;
  Cube::Side mRotation;

public:
  GameView* view;
  
public:
  Enemy();
  
  bool IsActive() { return mDistance>0; }
  Vec2 Position() { return mStart + mProgress * kSideToUnit[mRotation]; }
  int CurrentFrame();
  Vec2 Tile() { return mTile; }
  
  void Init(Vec2 tile, Vec2 start, Vec2 end);
  void Update(float dt);
  void Dispose();
  
};