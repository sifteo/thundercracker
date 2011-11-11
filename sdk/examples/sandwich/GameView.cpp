#include "GameView.h"
#include "Game.h"
#include "Enemy.h"

GameView::GameView() : 
visited(0), mRoom(-2,-2), mSpriteCount(0) {
}

void GameView::Init() {
  VidMode_BG0 mode(cube.vbuf);
  mode.init();
  if (gGame.player.CurrentView() == this) {
    ShowRoom(Vec2(0,0));
    ShowPlayer();
  } else {
    ShowRoom(ROOM_NONE);
  }
}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

bool GameView::IsShowingRoom() const {
  return mRoom.x >= 0 && 
    mRoom.y >= 0 && 
    mRoom.x < gGame.map.Data()->width && 
    mRoom.y < gGame.map.Data()->height; 
}

MapRoom* GameView::GetMapRoom() const { 
  ASSERT(IsShowingRoom()); 
  return gGame.map.GetRoom(mRoom.x, mRoom.y); 
}

void GameView::ShowRoom(Vec2 room) {
  if (mRoom == room) { return; }
  mRoom = room;
  // are we showing an enemy?
  for(Enemy* p = gGame.EnemyBegin(); p!=gGame.EnemyEnd(); ++p) {
    if (p->IsActive() && p->Tile() == mRoom) {
      ShowEnemy(p);
    } else if (p->view == this) {
      HideEnemy(p);
    }
  }
  
  DrawBackground();
}

void GameView::ClearRoom() {
  for(Enemy* p = gGame.EnemyBegin(); p!=gGame.EnemyEnd(); ++p) {
    if (p->view == this) {
      HideEnemy(p);
    }
  }
  mRoom = ROOM_NONE;
  DrawBackground();
}

//----------------------------------------------------------------
// PLAYER METHODS
//----------------------------------------------------------------


void GameView::ShowPlayer() {
  EnterSpriteMode();
  ResizeSprite(PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void GameView::UpdatePlayer() {
  Vec2 localPosition = gGame.player.Position() - 128 * mRoom;
  SetSpriteImage(PLAYER_SPRITE_ID, gGame.player.CurrentFrame());
  MoveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void GameView::HidePlayer() {
  ResizeSprite(PLAYER_SPRITE_ID, 0, 0);
  ExitSpriteMode();
}

//----------------------------------------------------------------------
// ENEMY METHODS
//----------------------------------------------------------------------

void GameView::ShowEnemy(Enemy* pEnemy) {
  EnterSpriteMode();
  ResizeSprite(ENEMY_SPRITE_ID, 32, 32);
  pEnemy->view = this;
  UpdateEnemy(pEnemy);
}

void GameView::UpdateEnemy(Enemy* pEnemy) {
  SetSpriteImage(ENEMY_SPRITE_ID, pEnemy->CurrentFrame());
  Vec2 localPosition = pEnemy->Position();
  MoveSprite(ENEMY_SPRITE_ID, localPosition.x-16, localPosition.y-16);
  
}

void GameView::HideEnemy(Enemy* pEnemy) {
  if (pEnemy->view == this) {
    ResizeSprite(ENEMY_SPRITE_ID, 0, 0);
    ExitSpriteMode();
    pEnemy->view = 0;
  }
}
//----------------------------------------------------------------------
// ACCELEROMETER SHTUFF
//----------------------------------------------------------------------

#define TILT_THRESHOLD 50

Cube::Side GameView::VirtualTiltDirection() const {
  Vec2 accel = cube.virtualAccel();
  if (accel.y < -TILT_THRESHOLD) {
    return 0;
  } else if (accel.x < -TILT_THRESHOLD) {
    return 1;
  } else if (accel.y > TILT_THRESHOLD) {
    return 2;
  } else if (accel.x > TILT_THRESHOLD) {
    return 3;
  } else {
    return -1;
  }
}

//----------------------------------------------------------------------
// SPRITE STUFF - all yoinked from stars demo
//----------------------------------------------------------------------

bool GameView::InSpriteMode() const {
  uint8_t byte;
  _SYS_vbuf_peekb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), &byte);
  return byte == _SYS_VM_BG0_SPR_BG1;
}

bool GameView::EnterSpriteMode() {
  mSpriteCount++;
  if (mSpriteCount == 1) {
    DoEnterSpriteMode();
    return true;
  } 
  return false;
}

void GameView::SetSpriteImage(int id, int tile) {
  uint16_t word = VideoBuffer::indexWord(tile);
  uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                   sizeof(_SYSSpriteInfo)/2 * id ); 
  _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void GameView::HideSprite(int id) {
  ResizeSprite(id, 0, 0);
}

void GameView::ResizeSprite(int id, int px, int py) {
  uint8_t xb = -px;
  uint8_t yb = -py;
  uint16_t word = ((uint16_t)xb << 8) | yb;
  uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                   sizeof(_SYSSpriteInfo)/2 * id ); 
  _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void GameView::MoveSprite(int id, int px, int py) {
  uint8_t xb = -px;
  uint8_t yb = -py;
  uint16_t word = ((uint16_t)xb << 8) | yb;
  uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                   sizeof(_SYSSpriteInfo)/2 * id ); 
  _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

bool GameView::ExitSpriteMode() {
  if (mSpriteCount > 0) {
    mSpriteCount--;
    if (mSpriteCount == 0) {
      DoEnterSimpleMode();
    } else {
      DrawBackground();
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------
// HELPERS
//----------------------------------------------------------------------

GameView* GameView::VirtualNeighborAt(Cube::Side side) const {
  Cube::ID neighbor = cube.virtualNeighborAt(side);
  return neighbor == CUBE_ID_UNDEFINED ? 0 : gGame.views + neighbor;
}

void GameView::DrawBackground() {
  VidMode_BG0 mode(cube.vbuf);
  if (!IsShowingRoom()) {
    mode.BG0_drawAsset(Vec2(0,0), Blank);
  } else {
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        mode.BG0_drawAsset(
          Vec2(x<<1,y<<1),
          *(gGame.map.Data()->tileset),
          gGame.map.Data()->GetTileId(mRoom.x, mRoom.y, x, y)
        );
      }
    }
  }
}


void GameView::DoEnterSpriteMode() {
  // Clear BG1/SPR before switching modes
  _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
  _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_TILES/2, 0, 32);
  _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
  // Switch modes
  _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
}

void GameView::DoEnterSimpleMode() {
  VidMode_BG0 mode(cube.vbuf);
  mode.init();
  DrawBackground();
}
