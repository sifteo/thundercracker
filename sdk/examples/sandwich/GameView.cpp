#include "GameView.h"
#include "Game.h"
#include "Enemy.h"

#include "BG1Helper.h"

#define ROOM_NONE (Vec2(-1,-1))
#define ITEM_SPRITE_ID 0
#define PLAYER_SPRITE_ID 1
#define ENEMY_SPRITE_ID 2

GameView::GameView() : 
visited(0), mRoom(-2,-2) {
}

void GameView::Init() {
  EnterSpriteMode();
  if (gGame.player.CurrentView() == this) {
    ShowLocation(gGame.player.Location());
    ShowPlayer();
  } else {
    ShowLocation(ROOM_NONE);
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

MapRoom* GameView::Room() const { 
  ASSERT(IsShowingRoom()); 
  return gGame.map.GetRoom(mRoom); 
}

void GameView::ShowLocation(Vec2 room) {
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
  // are we showing an items?
  if (IsShowingRoom()) {
    MapRoom* mr = Room();
    if (mr->itemId) {
      ShowItem(mr->itemId);
    } else {
      HideItem();
    }
  } else {
    HideItem();
  }
  DrawBackground();
}

void GameView::HideRoom() {
  if (mRoom == ROOM_NONE) { return; }
  for(Enemy* p = gGame.EnemyBegin(); p!=gGame.EnemyEnd(); ++p) {
    if (p->view == this) {
      HideEnemy(p);
    }
  }
  HideItem();
  mRoom = ROOM_NONE;
  DrawBackground();
}

//----------------------------------------------------------------
// PLAYER METHODS
//----------------------------------------------------------------


void GameView::ShowPlayer() {
  ResizeSprite(PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void GameView::SetPlayerFrame(unsigned frame) {
  SetSpriteImage(PLAYER_SPRITE_ID, frame);
}

void GameView::UpdatePlayer() {
  Vec2 localPosition = gGame.player.Position() - 128 * mRoom;
  SetSpriteImage(PLAYER_SPRITE_ID, gGame.player.CurrentFrame());
  MoveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void GameView::HidePlayer() {
  ResizeSprite(PLAYER_SPRITE_ID, 0, 0);
}

//----------------------------------------------------------------------
// ENEMY METHODS
//----------------------------------------------------------------------

void GameView::ShowEnemy(Enemy* pEnemy) {
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
    pEnemy->view = 0;
  }
}

  
//----------------------------------------------------------------------
// ITEM METHODS
//----------------------------------------------------------------------

void GameView::ShowItem(int itemId) {
  SetSpriteImage(ITEM_SPRITE_ID, Items.index + (itemId - 1) * Items.width * Items.height);;
  ResizeSprite(ITEM_SPRITE_ID, 16, 16);
  Vec2 p = 16 * Room()->Data()->LocalCenter();
  MoveSprite(ITEM_SPRITE_ID, p.x-8, p.y);
}

void GameView::SetItemPosition(Vec2 p) {
  p += 16 * Room()->Data()->LocalCenter();
  MoveSprite(ITEM_SPRITE_ID, p.x-8, p.y);
}

void GameView::HideItem() {
  ResizeSprite(ITEM_SPRITE_ID, 0, 0);
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

void GameView::EnterSpriteMode() {
  // Clear BG1/SPR before switching modes
  _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
  _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_TILES/2, 0, 32);
  _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
  // Switch modes
  _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
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
    mode.BG0_drawAsset(Vec2(0,0), *(gGame.map.Data()->blankImage));
  } else {
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        mode.BG0_drawAsset(
          Vec2(x<<1,y<<1),
          *(gGame.map.Data()->tileset),
          gGame.map.Data()->GetTileId(mRoom, Vec2(x, y))
        );
      }
    }

    BG1Helper ovrly(cube);
    uint8_t *p = Room()->Data()->overlay;
    if (p) {
      while(*p != 0xff) {
        uint8_t pos = p[0];
        uint8_t frm = p[1];
        p+=2;
        if (pos != 0xff && frm != 0xff) {
          Vec2 position = Vec2(pos>>4, pos & 0xf);
          ovrly.DrawAsset(2*position, *(gGame.map.Data()->overlay), frm);
        }
      }
    }
    ovrly.Flush();
  }
}
