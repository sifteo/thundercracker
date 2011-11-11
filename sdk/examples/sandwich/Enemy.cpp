#include "Enemy.h"
#include "GameView.h"

Enemy::Enemy() : mTile(0,0), mStart(0,0), mEnd(0,0), mProgress(0), mDistance(0), mRotation(0), view(0) {
}

int Enemy::CurrentFrame() {
  int framesPerStrip = (Skeleton.frames/4);
  int frame = (mProgress/2) % framesPerStrip;
  int tilesPerFrame = Skeleton.width * Skeleton.height;
  int tilesPerStrip = tilesPerFrame * framesPerStrip;
  return Skeleton.index + mRotation * tilesPerStrip + frame * tilesPerFrame;
}

void Enemy::Init(Vec2 tile, Vec2 start, Vec2 end) {
  // invariant - start and end are on the same horizontal or vertical line, dist a mult of 
  mTile = tile;
  mStart = start;
  mEnd = end;
  Vec2 delta = mEnd - mStart;
  if (delta.x == 0) {
    if (delta.y > 0) {
      mDistance = delta.y;
      mRotation = 2;
    } else {
      mDistance = -delta.y;
      mRotation = 0;
    }
  } else {
    if (delta.x > 0) {
      mDistance = delta.x;
      mRotation = 3;
    } else {
      mDistance = -delta.x;
      mRotation = 1;
    }
  }
  mProgress = 0;
}

void Enemy::Update(float dt) {
  if (view) {
    mProgress += ENEMY_PIXELS_PER_FRAME;
    if (mProgress >= mDistance) {
      mProgress = 0;
      Vec2 tmp = mStart;
      mStart = mEnd;
      mEnd = tmp;
      mRotation = (mRotation+2)%NUM_SIDES;
    }
    view->UpdateEnemy(this);
  }
}

void Enemy::Dispose() {
  if (view) view->HideEnemy(this);
  mStart = Vec2(0,0);
  mEnd = Vec2(0,0);
  mDistance = 0;
  mRotation = 0;
  mProgress = 0;
}