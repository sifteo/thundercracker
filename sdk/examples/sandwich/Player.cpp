#include "Player.h"
#include "GameView.h"
#include "Game.h"

#define DOOR_PAD 10
#define FRAMES_PER_CYCLE 6
#define GAME_FRAMES_PER_ANIM_FRAME 2

Player::Player() : mStatus(PLAYER_STATUS_IDLE),
pCurrent(gGame.views), pTarget(0), mPosition(64,64),
mDir(2), mKeyCount(0), mAnimFrame(0), mProgress(0), mNextDir(-1), 
mApproachingLockedDoor(false) {
  CORO_RESET;
}

void Player::Reset() {
  CORO_RESET;
  mPath.steps[0] = -1;
  mDir = 2;
  mProgress = 0;
  mNextDir = -1;
  mPosition = Vec2(64,64);
  mStatus = PLAYER_STATUS_IDLE;
  mApproachingLockedDoor = false;
}

MapRoom* Player::Room() const {
  return gGame.map.GetRoom(Location());
}

void Player::SetLocation(Vec2 position, Cube::Side direction) {
  CORO_RESET;
  mPath.steps[0] = -1;
  mDir = direction;
  mProgress = 0;
  mNextDir = -1;
  mPosition = position;
  mStatus = PLAYER_STATUS_IDLE;
  mApproachingLockedDoor = false;
}

void Player::Move(int dx, int dy) {
  mStatus = PLAYER_STATUS_WALKING;
  mPosition.x += dx;
  mPosition.y += dy;
  mProgress += abs(dx) + abs(dy);
  mDir = InferDirection(Vec2(dx, dy));
}

int Player::CurrentFrame() {
  switch(mStatus) {
    case PLAYER_STATUS_IDLE:
      return PlayerStand.index + mDir * (PlayerStand.width * PlayerStand.height);
    case PLAYER_STATUS_WALKING:
      int frame = mAnimFrame / GAME_FRAMES_PER_ANIM_FRAME;
      int tilesPerFrame = PlayerWalk.width * PlayerWalk.height;
      int tilesPerStrip = tilesPerFrame * FRAMES_PER_CYCLE;
      return PlayerWalk.index + mDir * tilesPerStrip + frame * tilesPerFrame;
  }
  return 0;
}

void Player::Update(float dt) {
  // every update code here
  if (mStatus == PLAYER_STATUS_WALKING) {
    mAnimFrame = (mAnimFrame + 1) % (GAME_FRAMES_PER_ANIM_FRAME * FRAMES_PER_CYCLE);
  }

  CORO_BEGIN;
  // stately update code here
  for(;;) {
    // wait for tilt
    mStatus = PLAYER_STATUS_IDLE;
    pCurrent->UpdatePlayer();
    mNextDir = pCurrent->VirtualTiltDirection();
    mPath.Cancel();
    while(!gGame.map.CanTraverse(pCurrent->Location(),mNextDir) || !(pTarget=pCurrent->VirtualNeighborAt(mNextDir))) {
      CORO_YIELD;
      mNextDir = pCurrent->VirtualTiltDirection();
      if (mNextDir != -1) {
        mDir = mNextDir;
        pCurrent->UpdatePlayer();
      } else if (PathDetect()) {
        break;
      }
    }
    // go to the target
    mStatus = PLAYER_STATUS_WALKING;
    mDir = mNextDir;
    do {
      if (mPath.IsDefined()) {
        mDir = mPath.steps[0];
        pTarget = pCurrent->VirtualNeighborAt(mDir);
      }
      // animate walking to target
      pTarget->ShowPlayer();
      ASSERT( gGame.map.FindPath(pCurrent->Location(), mDir, &mMoves) );
      mProgress = 0;
      for(pNextMove=mMoves.pFirstMove; pNextMove!=mMoves.End(); ++pNextMove) {
        mDir = *pNextMove;  
        if (mProgress != 0) {
          mPosition += mProgress * kSideToUnit[*pNextMove];
          mProgress = 0;
          pCurrent->UpdatePlayer();
          pTarget->UpdatePlayer();
          CORO_YIELD;
        }
        while(mProgress+WALK_SPEED < 16) {
          mProgress += WALK_SPEED;
          mPosition += WALK_SPEED * kSideToUnit[*pNextMove];
          pCurrent->UpdatePlayer();
          pTarget->UpdatePlayer();
          CORO_YIELD;
        }
        mPosition += (16 - mProgress) * kSideToUnit[*pNextMove];
        mProgress = WALK_SPEED - (16-mProgress);
      }
      if (mProgress != 0) {
        pNextMove--;
        mPosition += mProgress * kSideToUnit[*pNextMove];
        mProgress = 0;
        pCurrent->UpdatePlayer();
        pTarget->UpdatePlayer();
        CORO_YIELD;
      }
      pCurrent->HidePlayer();
      pCurrent = pTarget;
      pTarget = 0;  
      pCurrent->UpdatePlayer();
      { // passive trigger?
        MapRoom *mr = pCurrent->Room();
        if (mr->callback) {
          mr->callback(TRIGGER_TYPE_PASSIVE);
        }
      }
      
      /* // a reference for how doors used to be implemented
      mApproachingLockedDoor = pCurrent->Room()->GetPortal(mDir) == PORTAL_LOCK;
      if (!mApproachingLockedDoor || HaveBasicKey()) {
        for(mProgress=0; mProgress<128; mProgress+=WALK_SPEED) {
          if (mApproachingLockedDoor && mProgress < 64-DOOR_PAD && mProgress+WALK_SPEED >= 64-DOOR_PAD) {
            DecrementBasicKeyCount();
            pCurrent->Room()->SetPortal(mDir, PORTAL_DOOR);
            pCurrent->Room()->OpenDoor(mDir);
            pTarget->Room()->OpenDoor((mDir+2)%4);
            pCurrent->DrawBackground();
            pTarget->DrawBackground();
            mTimeout = System::clock();
            do {
              CORO_YIELD;
            } while(System::clock() - mTimeout <  0.5f);
          }
          CORO_YIELD;
          mPosition += WALK_SPEED * kSideToUnit[mDir];
          pCurrent->UpdatePlayer();
          pTarget->UpdatePlayer();
        }
        // swap the current view
        pCurrent->HidePlayer();
        pCurrent = pTarget;
        pTarget = 0;  
        pCurrent->UpdatePlayer();

        { // passive trigger?
          MapRoom *mr = pCurrent->Room();
          if (mr->callback) {
            mr->callback(TRIGGER_TYPE_PASSIVE);
          }
        } //
      } else {
        // walk up to the locked door, then bounce back
        mPath.Cancel();
        pTarget->HidePlayer();
        pTarget = 0;
        for(mProgress=0; mProgress<64-DOOR_PAD; mProgress+=WALK_SPEED) {
          CORO_YIELD;
          mPosition += WALK_SPEED * kSideToUnit[mDir];
          pCurrent->UpdatePlayer();
        }
        mDir = (mDir+2)%4;
        for(mProgress=0; mProgress<64-DOOR_PAD; mProgress+=WALK_SPEED) {
          CORO_YIELD;
          mPosition += WALK_SPEED * kSideToUnit[mDir];
          pCurrent->UpdatePlayer();
        }
      }
      */ 

    } while(mPath.PopStep(pCurrent));

    { // active trigger?
      MapRoom *mr = pCurrent->Room();
      if (mr->callback) {
        mr->callback(TRIGGER_TYPE_ACTIVE);
      }
    }

    mDir = SIDE_BOTTOM;
    pCurrent->UpdatePlayer();
  }
  
  CORO_END;
}

//------------------------------------------------------------------------
// PATH DETECTION
//------------------------------------------------------------------------

Path::Path() {
  for(int i=0; i<NUM_CUBES-1; ++i) {
    steps[i] = -1;
  }
}

bool Path::PopStep(GameView* newRoot) {
  if (steps[0] == -1 || steps[1] == -1) {
    steps[0] = -1;
    return false;
  }
  for(int i=0; i<NUM_CUBES-2; ++i) {
    steps[i] = steps[i+1];
  }
  steps[NUM_CUBES-2] = -1;
  if (*steps != -1 && newRoot->VirtualNeighborAt(*steps)) {
    return true;
  }
  steps[0] = -1;
  return false;
}

void Path::Cancel() {
  steps[0] = -1;
}

bool Player::PathDetect() {
  if (pTarget) { return false; }
  for(GameView *p = gGame.ViewBegin(); p!=gGame.ViewEnd(); ++p) { p->visited = false; }
  pCurrent->visited = true;
  for(int side=0; side<NUM_SIDES; ++side) {
    mPath.steps[0] = side;
    if (gGame.map.CanTraverse(pCurrent->Location(), side) && PathVisit(pCurrent->VirtualNeighborAt(side), 1)) {
      return true;
    }
  }
  mPath.steps[0] = -1;
  return false;
}

bool Player::PathVisit(GameView* view, int depth) {
  if (!view || view->visited) { return false; }
  view->visited = true;
  if (view->VirtualTiltDirection() != -1) {
    mPath.steps[depth] = -1;
    return true;
  } else {
    for(int side=0; side<NUM_SIDES; ++side) {
      mPath.steps[depth] = side;
      if (gGame.map.CanTraverse(view->Location(), side) && PathVisit(view->VirtualNeighborAt(side), depth+1)) {
        return true;
      } else {
        mPath.steps[depth] = -1;
      }
    }
  }
  mPath.steps[depth] = -1;
  return false;
}


