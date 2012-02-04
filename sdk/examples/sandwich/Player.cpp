#include "Player.h"
#include "RoomView.h"
#include "Game.h"
#include "Dialog.h"

#define DOOR_PAD 10
#define GAME_FRAMES_PER_ANIM_FRAME 2

inline int fast_abs(int x) {
	return x<0?-x:x;
}

void Player::Init() {
  const RoomData& room = gMapData[gQuestData->mapId].rooms[gQuestData->roomId];
  mCurrent.view = (RoomView*)(pGame->ViewBegin());
  mCurrent.subdivision = 0;
  mTarget.view = 0;
  mPosition.x = 128 * (gQuestData->roomId % gMapData[gQuestData->mapId].width) + 16 * room.centerX;
  mPosition.y = 128 * (gQuestData->roomId / gMapData[gQuestData->mapId].width) + 16 * room.centerY;
  mStatus = PLAYER_STATUS_IDLE;
  mDir = 2;
  mKeyCount = 0;
  mItemMask = 0;
  mAnimFrame = 0;
  mAnimTime = 0.f;
  mProgress = 0;
  mNextDir = -1;
  mApproachingLockedDoor = false;
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

Room* Player::CurrentRoom() const {
  return pGame->GetMap()->GetRoom(Location());
}

void Player::PickupItem(int itemId) {
  if (itemId == 0) { return; }
  PlaySfx(sfx_pickup);
  if (itemId == ITEM_BASIC_KEY || itemId == ITEM_SKELETON_KEY) {
    mKeyCount++;
    if (mKeyCount == 1) {
      pGame->OnInventoryChanged();
    }
  } else if (!HasItem(itemId)) {
    mItemMask |= (1<<itemId);
    ASSERT(HasItem(itemId));
    pGame->OnInventoryChanged();
  }
}

void Player::DecrementBasicKeyCount() { 
  ASSERT(mKeyCount>0); 
  mKeyCount--; 
  if (mKeyCount == 0) {
    pGame->OnInventoryChanged();
  }
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
  mProgress += fast_abs(dx) + fast_abs(dy);
  mDir = InferDirection(Vec2(dx, dy));
}

int Player::AnimFrame() {
  switch(mStatus) {
    case PLAYER_STATUS_IDLE: {
      if (mAnimFrame == 1) {
        return PlayerIdle.index;
      } else if (mAnimFrame == 2) {
        return PlayerIdle.index + 16;
      } else if (mAnimFrame == 3 || mAnimFrame == 4) {
        return PlayerIdle.index + (mAnimFrame-1) * 16;
      } else {
        return PlayerStand.index + SIDE_BOTTOM * 16;
      }
    }
    case PLAYER_STATUS_WALKING:
      int frame = mAnimFrame / GAME_FRAMES_PER_ANIM_FRAME;
      int tilesPerFrame = PlayerWalk.width * PlayerWalk.height;
      int tilesPerStrip = tilesPerFrame * (PlayerWalk.frames>>2);
      return PlayerWalk.index + mDir * tilesPerStrip + frame * tilesPerFrame;
  }
  return 0;
}

void Player::UpdateAnimation(float dt) {
  if (mStatus == PLAYER_STATUS_WALKING) {
    mAnimFrame = (mAnimFrame + 1) % (GAME_FRAMES_PER_ANIM_FRAME * (PlayerWalk.frames>>2));
  } else { // PLAYER_STATUS_IDLE
    mAnimTime += dt;
    if (mAnimTime >= 10.f) {
      mAnimTime -= 10.f;
    }
    int before = mAnimFrame;
    if (mAnimTime > 0.5f && mAnimTime < 1.f) {
      mAnimFrame = 1;
    } else if (mAnimTime > 2.5f && mAnimTime < 3.f) {
      mAnimFrame = 2;
    } else if (mAnimTime > 5.f && mAnimTime < 5.5f) {
      mAnimFrame = (mAnimTime < 5.25f ? 3 : 4);
    } else {
      mAnimFrame = 0;
    }
    if (before != mAnimFrame) {
      mCurrent.view->UpdatePlayer();
    }
  }  
}

void Player::Update(float dt) {
  // every update code here
  UpdateAnimation(dt);

  CORO_BEGIN;
  // stately update code here
  for(;;) {
    // wait for tilt
    mStatus = PLAYER_STATUS_IDLE;
    mCurrent.view->UpdatePlayer();
    mNextDir = mCurrent.view->Parent()->VirtualTiltDirection();
    mPath.Cancel();
    mAnimFrame = 0;
    mAnimTime = 0.f;
    #if TOUCH_ONLY
      do {
        CORO_YIELD;
      } while (!pGame->GetMap()->FindBroadPath(&mPath));
    #else
      do {
        CORO_YIELD;
        mNextDir = mCurrent.view->VirtualTiltDirection();
      } while(mNextDir == -1 || !pGame->GetMap()->GetBroadLocationNeighbor(mCurrent, mNextDir, &mTarget));
    #endif
    // go to the target
    mStatus = PLAYER_STATUS_WALKING;
    mAnimFrame = 0;
    mDir = mNextDir;
    do {
      if (mPath.IsDefined()) {
        mDir = mPath.steps[0];
        pGame->GetMap()->GetBroadLocationNeighbor(mCurrent, mDir, &mTarget);
      }
      // animate walking to target
      PlaySfx(sfx_running);
      mTarget.view->ShowPlayer();
      if (mDir == SIDE_TOP && mCurrent.view->GetRoom()->HasClosedDoor()) {
        for(mProgress=0; mProgress<24; mProgress+=WALK_SPEED) {
          mPosition.y -= WALK_SPEED;
          mCurrent.view->UpdatePlayer();
          CORO_YIELD;
        }
        if (HaveBasicKey()) {
          DecrementBasicKeyCount();
          // check the door
          mCurrent.view->GetRoom()->OpenDoor();
          mCurrent.view->DrawBackground();
          mCurrent.view->Parent()->GetCube()->vbuf.touch();
          mCurrent.view->UpdatePlayer();
          mTimeout = System::clock();
          pGame->NeedsSync();
          do {
            CORO_YIELD;
          } while(System::clock() - mTimeout <  0.5f);
          PlaySfx(sfx_doorOpen);
          // finish up
          for(; mProgress+WALK_SPEED<=128; mProgress+=WALK_SPEED) {
            mPosition.y -= WALK_SPEED;
            mCurrent.view->UpdatePlayer();
            mTarget.view->UpdatePlayer();
            CORO_YIELD;
          }
          // fill in the remainder
          mPosition = mTarget.view->GetRoom()->Center(mTarget.subdivision);
        } else {
          PlaySfx(sfx_doorBlock);
          mPath.Cancel();
          mTarget.view->HidePlayer();
          mTarget.view = 0;
          mDir = (mDir+2)%4;
          for(mProgress=0; mProgress<24; mProgress+=WALK_SPEED) {
            CORO_YIELD;
            mPosition.y += WALK_SPEED;
            mCurrent.view->UpdatePlayer();
          }          
        }
      } else { // general case - A*
        if (mTarget.view->GetRoom()->IsBridge()) {
          mTarget.view->HideOverlay(mDir%2 == 1);
        }
  			{bool result = pGame->GetMap()->FindNarrowPath(mCurrent, mDir, &mMoves);
	   		ASSERT(result);}
        mProgress = 0;
        for(pNextMove=mMoves.pFirstMove; pNextMove!=mMoves.End(); ++pNextMove) {
          mDir = *pNextMove;  
          if (mProgress != 0) {
            mPosition += mProgress * kSideToUnit[*pNextMove];
            mCurrent.view->UpdatePlayer();
            mTarget.view->UpdatePlayer();
            CORO_YIELD;
          }
          while(mProgress+WALK_SPEED < 16) {
            mProgress += WALK_SPEED;
            mPosition += WALK_SPEED * kSideToUnit[*pNextMove];
            mCurrent.view->UpdatePlayer();
            mTarget.view->UpdatePlayer();
            CORO_YIELD;
          }
          mPosition += (16 - mProgress) * kSideToUnit[*pNextMove];
          mProgress = WALK_SPEED - (16-mProgress);
        }
        if (mProgress != 0) {
          pNextMove--;
          mPosition += mProgress * kSideToUnit[*pNextMove];
          mProgress = 0;
          mCurrent.view->UpdatePlayer();
          mTarget.view->UpdatePlayer();
          CORO_YIELD;
        }
      }
      if (mTarget.view) { // did we land on the target?
        mCurrent.view->HidePlayer();
        mCurrent = mTarget;
        mTarget.view = 0;  
        mPosition = mCurrent.view->GetRoom()->Center(mCurrent.subdivision);
        mCurrent.view->UpdatePlayer();        
        if (mCurrent.view->GetRoom()->HasItem()) {
          const ItemData* pItem = mCurrent.view->GetRoom()->TriggerAsItem();
          if (pGame->GetState()->FlagTrigger(pItem->trigger)) { mCurrent.view->GetRoom()->ClearTrigger(); }
          PickupItem(pItem->itemId);
          // do a pickup animation
          for(unsigned frame=0; frame<PlayerPickup.frames; ++frame) {
            mCurrent.view->SetPlayerFrame(PlayerPickup.index + (frame * PlayerPickup.width * PlayerPickup.height));
            
            for(float t=System::clock(); System::clock()-t<0.075f;) {
              // this calc is kinda annoyingly complex
              float u = (System::clock() - t) / 0.075f;
              const float du = 1.f / (float) PlayerPickup.frames;
              u = (frame + u) * du;
              u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
              pGame->Paint();
              mCurrent.view->SetItemPosition(Vec2(0, -36.f * u) );
            }
          }
          mCurrent.view->SetPlayerFrame(PlayerStand.index+ SIDE_BOTTOM* PlayerStand.width * PlayerStand.height);
          for(float t=System::clock(); System::clock()-t<0.25f;) { System::paint(); }
          mCurrent.view->HideItem();        
        }
      }
    } while(mPath.PopStep(mCurrent, &mTarget));
    if (mCurrent.view->GetRoom()->HasGateway()) {
      const GatewayData* pGate = mCurrent.view->GetRoom()->TriggerAsGate();
      const MapData& targetMap = gMapData[pGate->targetMap];
      const GatewayData& pTargetGate = targetMap.gates[pGate->targetGate];
      if (pGame->GetState()->FlagTrigger(pGate->trigger)) { mCurrent.view->GetRoom()->ClearTrigger(); } 
      pGame->WalkTo(128 * mCurrent.view->GetRoom()->Location() + Vec2(pGate->x, pGate->y));
      pGame->TeleportTo(gMapData[pGate->targetMap], Vec2(
        128 * (pTargetGate.trigger.room % targetMap.width) + pTargetGate.x,
        128 * (pTargetGate.trigger.room / targetMap.width) + pTargetGate.y
      ));
    } else if (mCurrent.view->GetRoom()->HasNPC()) {
      ////////
      mStatus = PLAYER_STATUS_IDLE;
      mCurrent.view->UpdatePlayer();
      for(int i=0; i<16; ++i) {
        pGame->Paint(true);
      }
      const NpcData* pNpc = mCurrent.view->GetRoom()->TriggerAsNPC();
      if (pGame->GetState()->FlagTrigger(pNpc->trigger)) { mCurrent.view->GetRoom()->ClearTrigger(); }
      DoDialog(gDialogData[pNpc->dialog], mCurrent.view->Parent()->GetCube());
      System::paintSync();
      mCurrent.view->Init(pGame->GetMap()->GetRoomId(Location()));
      System::paintSync();
    }
    mDir = SIDE_BOTTOM;
    mCurrent.view->UpdatePlayer();
  }
  
  CORO_END;
}
