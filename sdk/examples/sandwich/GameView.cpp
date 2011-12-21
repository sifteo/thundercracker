#include "GameView.h"
#include "Game.h"
#include "Enemy.h"

#define ROOM_NONE (Vec2(-1,-1))
#define ITEM_SPRITE_ID 0
#define PLAYER_SPRITE_ID 1
#define ENEMY_SPRITE_ID 2

GameView::GameView() : 
visited(0), mRoom(-2,-2) {
}

void GameView::Init() {
  EnterSpriteMode(GetCube());
  if (pGame->player.CurrentView() == this) {
    mRoom = Vec2(-1,-1);
    ShowLocation(pGame->player.Location());
  } else {
    ShowLocation(ROOM_NONE);
  }
}

Cube* GameView::GetCube() const {
  int id = (int)(this - pGame->ViewBegin());
  return cubes + id;
}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

bool GameView::IsShowingRoom() const {
  return mRoom.x >= 0 && 
    mRoom.y >= 0 && 
    mRoom.x < pGame->map.Data()->width && 
    mRoom.y < pGame->map.Data()->height; 
}

MapRoom* GameView::Room() const { 
  ASSERT(IsShowingRoom()); 
  return pGame->map.GetRoom(mRoom); 
}

void GameView::ShowLocation(Vec2 room) {
  if (room == mRoom) { return; }
  mRoom = room;
  // are we showing an enemy?
  /*
  for(Enemy* p = pGame->EnemyBegin(); p!=pGame->EnemyEnd(); ++p) {
    if (p->IsActive() && p->Tile() == mRoom) {
      ShowEnemy(p);
    } else if (p->view == this) {
      HideEnemy(p);
    }
  }
  */
  // are we showing an items?
  if (IsShowingRoom()) {
    HideInventorySprites();
    MapRoom* mr = Room();
    if (mr->itemId) {
      ShowItem(mr->itemId);
    }
    if (this == pGame->player.CurrentView()) {
      ShowPlayer();
    }
    DrawBackground();
  } else {
    HideRoom();
  }
  pGame->NeedsSync();
}

void GameView::HideRoom() {
  /*
  for(Enemy* p = pGame->EnemyBegin(); p!=pGame->EnemyEnd(); ++p) {
    if (p->view == this) {
      HideEnemy(p);
    }
  }
  */
  mRoom = ROOM_NONE;
  HideItem();
  DrawInventorySprites();
  DrawBackground();
}

//----------------------------------------------------------------
// PLAYER METHODS
//----------------------------------------------------------------


void GameView::ShowPlayer() {
  ResizeSprite(GetCube(), PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void GameView::SetPlayerFrame(unsigned frame) {
  SetSpriteImage(GetCube(), PLAYER_SPRITE_ID, frame);
}

void GameView::UpdatePlayer() {
  Vec2 localPosition = pGame->player.Position() - 128 * mRoom;
  SetSpriteImage(GetCube(), PLAYER_SPRITE_ID, pGame->player.CurrentFrame());
  MoveSprite(GetCube(), PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void GameView::HidePlayer() {
  ResizeSprite(GetCube(), PLAYER_SPRITE_ID, 0, 0);
}

//----------------------------------------------------------------------
// ENEMY METHODS
//----------------------------------------------------------------------

void GameView::ShowEnemy(Enemy* pEnemy) {
  ResizeSprite(GetCube(), ENEMY_SPRITE_ID, 32, 32);
  pEnemy->view = this;
  UpdateEnemy(pEnemy);
}

void GameView::UpdateEnemy(Enemy* pEnemy) {
  SetSpriteImage(GetCube(), ENEMY_SPRITE_ID, pEnemy->CurrentFrame());
  Vec2 localPosition = pEnemy->Position();
  MoveSprite(GetCube(), ENEMY_SPRITE_ID, localPosition.x-16, localPosition.y-16);
  
}

void GameView::HideEnemy(Enemy* pEnemy) {
  if (pEnemy->view == this) {
    HideSprite(GetCube(), ENEMY_SPRITE_ID);
    pEnemy->view = 0;
  }
}

  
//----------------------------------------------------------------------
// ITEM METHODS
//----------------------------------------------------------------------

void GameView::ShowItem(int itemId) {
  SetSpriteImage(GetCube(), ITEM_SPRITE_ID, Items.index + (itemId - 1) * Items.width * Items.height);;
  ResizeSprite(GetCube(), ITEM_SPRITE_ID, 16, 16);
  Vec2 p = 16 * Room()->Data()->LocalCenter();
  MoveSprite(GetCube(), ITEM_SPRITE_ID, p.x-8, p.y);
}

void GameView::SetItemPosition(Vec2 p) {
  p += 16 * Room()->Data()->LocalCenter();
  MoveSprite(GetCube(), ITEM_SPRITE_ID, p.x-8, p.y);
}

void GameView::HideItem() {
  ResizeSprite(GetCube(), ITEM_SPRITE_ID, 0, 0);
}


void GameView::RefreshInventory() {
  if (!IsShowingRoom()) {
    DrawInventorySprites();
  }
}

void GameView::DrawInventorySprites() {
  int count = 0;
  const int pad = 24;
  const int innerPad = (128-pad-pad)/3;
  const int firstSandwichId = 2;
  for(int itemId=firstSandwichId; itemId<firstSandwichId+4; ++itemId) {
    if (pGame->player.HasItem(itemId)) {
      ResizeSprite(GetCube(), count, 16, 16);
      MoveSprite(GetCube(), count, pad + innerPad * count - 8, 108);
      SetSpriteImage(GetCube(), count, Items.index + (itemId-1) * Items.width * Items.height);
      count++;
    }
  }
  if (pGame->player.HaveBasicKey()) {
      ResizeSprite(GetCube(), count, 16, 16);
      MoveSprite(GetCube(), count, pad + innerPad * count - 8, 108);
      SetSpriteImage(GetCube(), count, Items.index + (ITEM_BASIC_KEY-1) * Items.width * Items.height);
      count++;
  }
  for(int i=count; i<4; ++i) {
    HideSprite(GetCube(), i);
  }
}

void GameView::HideInventorySprites() {
  for(int i=0; i<4; ++i) {
    HideSprite(GetCube(), i);
  }
}

//----------------------------------------------------------------------
// ACCELEROMETER SHTUFF
//----------------------------------------------------------------------

#define TILT_THRESHOLD 50

Cube::Side GameView::VirtualTiltDirection() const {
  Vec2 accel = GetCube()->virtualAccel();
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
// HELPERS
//----------------------------------------------------------------------

GameView* GameView::VirtualNeighborAt(Cube::Side side) const {
  Cube::ID neighbor = GetCube()->virtualNeighborAt(side);
  return neighbor == CUBE_ID_UNDEFINED ? 0 : pGame->views + neighbor;
}

void GameView::DrawBackground() {
  VidMode_BG0 mode(GetCube()->vbuf);
  if (!IsShowingRoom()) {
    mode.BG0_drawAsset(Vec2(0,0), *(pGame->map.Data()->blankImage));
    BG1Helper(*GetCube()).Flush();
  } else {
    for(int y=0; y<8; ++y) {
      for(int x=0; x<8; ++x) {
        mode.BG0_drawAsset(
          Vec2(x<<1,y<<1),
          *(pGame->map.Data()->tileset),
          pGame->map.Data()->GetTileId(mRoom, Vec2(x, y))
        );
      }
    }

    BG1Helper ovrly(*GetCube());
    uint8_t *p = Room()->Data()->overlay;
    if (p) {
      while(*p != 0xff) {
        uint8_t pos = p[0];
        uint8_t frm = p[1];
        p+=2;
        if (pos != 0xff && frm != 0xff) {
          Vec2 position = Vec2(pos>>4, pos & 0xf);
          ovrly.DrawAsset(2*position, *(pGame->map.Data()->overlay), frm);
        }
      }
    }
    ovrly.Flush();
  }
}
