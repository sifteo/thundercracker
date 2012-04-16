#include "Player.h"
#include "RoomView.h"
#include "Game.h"

#define DOOR_PAD 10
#define GAME_FRAMES_PER_ANIM_FRAME 2

void Player::Init(Viewport* pPrimary) {
  const RoomData& room = gMapData[gQuestData->mapId].rooms[gQuestData->roomId];
  mCurrent.view = reinterpret_cast<RoomView*>(pPrimary);
  mCurrent.subdivision = 0;
  mTarget.view = 0;
  mPosition.x = 128 * (gQuestData->roomId % gMapData[gQuestData->mapId].width) + 16 * room.centerX;
  mPosition.y = 128 * (gQuestData->roomId / gMapData[gQuestData->mapId].width) + 16 * room.centerY;
  mStatus = PLAYER_STATUS_IDLE;
  mDir = 2;
  mAnimFrame = 0;
  mAnimTime = 0.f;
  mEquipment = 0;
  mImageStrip = &PlayerStand;
  ComputeFrame();
}

Room* Player::GetRoom() const {
  return &gGame.GetMap().GetRoom(Location());
}

void Player::ConsumeEquipment() {
  ASSERT(mEquipment);
  gGame.GetState().FlagTrigger(mEquipment->trigger);
  mEquipment = 0;
}

void Player::Move(int dx, int dy) { 
  SetStatus(PLAYER_STATUS_WALKING);
  mPosition.x += dx;
  mPosition.y += dy;
  if (mCurrent.view) { mCurrent.view->UpdatePlayer(); }
  if (mTarget.view) { mTarget.view->UpdatePlayer(); }
}

void Player::ClearTarget() {
  mTarget.view->HidePlayer();
  mTarget.view = 0;
}

void Player::AdvanceToTarget() {
  mCurrent.view->HidePlayer();
  mCurrent = mTarget;
  mTarget.view = 0;  
  mPosition = GetRoom()->Center(mCurrent.subdivision);
  mCurrent.view->UpdatePlayer();
}

void Player::SetStatus(int status) {
  if (mStatus == status) { return; }
  mStatus = status;
  mAnimTime = 0.f;
  mAnimFrame = 0;
  ComputeFrame();
}

// the following stuff is kinda hacky because it's been iterated on so much
// I'll fix it up later, I promiiiiseeeee

void Player::ComputeFrame() {
  switch(mStatus) {
    case PLAYER_STATUS_IDLE:
      if (mAnimFrame) {
        mSpriteFrame = mAnimFrame-1;
        mImageStrip = &PlayerIdle;
      } else {
        mSpriteFrame = 2;
        mImageStrip = &PlayerStand;
      }
      break;
    
    case PLAYER_STATUS_WALKING:
      mSpriteFrame = mDir * (PlayerWalk.numFrames()>>2) + (mAnimFrame / GAME_FRAMES_PER_ANIM_FRAME);
      mImageStrip = &PlayerWalk;
      break;
  }
}

void Player::Update() {
  switch(mStatus) {
    
    case PLAYER_STATUS_WALKING:
      mAnimFrame = (mAnimFrame + 1) % (GAME_FRAMES_PER_ANIM_FRAME * PlayerWalk.numFrames()/4);
      ComputeFrame();
      break;
    
    case PLAYER_STATUS_IDLE:
      mAnimTime = fmod(mAnimTime+gGame.Dt().seconds(), 10.f);
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
      ComputeFrame();
      if (before != mAnimFrame) {
        mCurrent.view->UpdatePlayer();
      }
      break;
  }
}


