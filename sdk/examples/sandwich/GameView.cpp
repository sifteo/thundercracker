#include "GameView.h"
#include "Game.h"

// Constants

#define ROOM_NONE (Vec2(-1,-1))
#define BFF_SPRITE_ID 0
#define ITEM_SPRITE_ID 1
#define PLAYER_SPRITE_ID 2

static const int8_t sHoverTable[] = { 
  0, 0, 1, 2, 2, 3, 3, 3, 
  4, 3, 3, 3, 2, 2, 1, 0, 
  -1, -1, -2, -3, -3, -4, 
  -4, -4, -4, -4, -4, -4, 
  -3, -3, -2, -1
};
#define HOVER_COUNT 32

const static uint8_t sHopTable[] = { 0, 0, 0, 1, 3, 4, 6, 6, 7, 7, 8, 7, 7, 6, 6, 4, 3, 1, };
#define HOP_COUNT 18
#define HOP_PHASE 9

#define FRAMES_PER_TORCH_FRAME 4
#define BFF_FRAME_COUNT 4

namespace BffDir {
  enum ID { E, SE, S, SW, W, NW, N, NE };
}

static const Vec2 sBffTable[] = {
  Vec2(1, 0),
  Vec2(1, 1),
  Vec2(0, 1),
  Vec2(-1, 1),
  Vec2(-1, 0),
  Vec2(-1, -1),
  Vec2(0, -1),
  Vec2(1, -1),
};

// methods

GameView::GameView() : 
visited(0), mRoom(ROOM_NONE), mIdleHoverIndex(0) {
}

void GameView::Init() {
  EnterSpriteMode(GetCube());
  if (pGame->player.CurrentView() == this) {
    mRoom = Vec2(-1,-1);
    ShowLocation(pGame->player.Location());
  } else {
    mRoom.x = -2; // h4ck
    ShowLocation(ROOM_NONE);
  }
}

Cube* GameView::GetCube() const {
  int id = (int)(this - pGame->ViewBegin());
  return gCubes + id;
}

void GameView::Update() {
  if (!IsShowingRoom()) { 
    // compute position of each inventory item based on phase and current time
    if (mScene.idle.count > 0 && mScene.idle.time <= HOP_PHASE * (mScene.idle.count-1) + HOP_COUNT) {
      int count = 0;
      const int pad = 24;
      const int innerPad = (128-pad-pad)/3;
      for(int i=0; i<mScene.idle.count; ++i) {
        int localTime = mScene.idle.time - HOP_PHASE * i;
        if (localTime < HOP_COUNT) {
          MoveSprite(GetCube(), i, pad + innerPad * i - 8, 108  - sHopTable[localTime]);
        } else {
          MoveSprite(GetCube(), i, pad + innerPad * i - 8, 108);
        }
      }
      mScene.idle.time++;
    }
    return; 
  }

  // begin h4cky scene-specific stuff
  RoomData *p = Room()->Data();
  if (pGame->map.Data() == &dungeon_data) {
    if (p->torch0 != 0xff) {
      mScene.dungeon.torchTime = (mScene.dungeon.torchTime + 1) % (6 * FRAMES_PER_TORCH_FRAME);
      unsigned torchFrame = mScene.dungeon.torchTime / FRAMES_PER_TORCH_FRAME;
      if (torchFrame * FRAMES_PER_TORCH_FRAME == mScene.dungeon.torchTime) {
        // advancing the frame
        Vec2 tt0 = p->TorchTile0();
        VidMode_BG0 mode(GetCube()->vbuf);
        mode.BG0_drawAsset(
          Vec2(tt0.x<<1,tt0.y<<1),
          *(pGame->map.Data()->tileset),
          pGame->map.Data()->GetTileId(mRoom, tt0) + torchFrame
        );
        tt0.y++;
        mode.BG0_drawAsset(
          Vec2(tt0.x<<1,tt0.y<<1),
          *(pGame->map.Data()->tileset),
          pGame->map.Data()->GetTileId(mRoom, tt0) + torchFrame
        );
        if (p->torch1 != 0xff) {
          tt0 = p->TorchTile1();
          mode.BG0_drawAsset(
            Vec2(tt0.x<<1,tt0.y<<1),
            *(pGame->map.Data()->tileset),
            pGame->map.Data()->GetTileId(mRoom, tt0) + torchFrame
          );
          tt0.y++;
          mode.BG0_drawAsset(
            Vec2(tt0.x<<1,tt0.y<<1),
            *(pGame->map.Data()->tileset),
            pGame->map.Data()->GetTileId(mRoom, tt0) + torchFrame
          );
        }
      }
    }    
  } else if (pGame->map.Data() == &forest_data && mScene.forest.hasBff) {
    // butterfly stuff
    Vec2 delta = sBffTable[mScene.forest.bffDir];
    mScene.forest.bffX += (uint8_t) delta.x;
    mScene.forest.bffY += (uint8_t) delta.y;
    MoveSprite(GetCube(), BFF_SPRITE_ID, mScene.forest.bffX-68, mScene.forest.bffY-68);
    // hack - assumes butterflies and items are not rendered on same cube
    mIdleHoverIndex = (mIdleHoverIndex + 1) % (BFF_FRAME_COUNT * FRAMES_PER_TORCH_FRAME);
    SetSpriteImage(GetCube(), BFF_SPRITE_ID, 
      Butterfly.index + 4 * mScene.forest.bffDir + mIdleHoverIndex / FRAMES_PER_TORCH_FRAME
    );
    using namespace BffDir;
    switch(mScene.forest.bffDir) {
      case S:
        if (mScene.forest.bffY > 196) { RandomizeBff(); }
        break;
      case SW:
        if (mScene.forest.bffX < 60 || mScene.forest.bffY > 196) { RandomizeBff(); }
        break;
      case W:
        if (mScene.forest.bffX < 60) { RandomizeBff(); }
        break;
      case NW:
        if (mScene.forest.bffX < 60 || mScene.forest.bffY < 60) { RandomizeBff(); }
        break;
      case N:
      if (mScene.forest.bffY < 60) { RandomizeBff(); }
        break;
      case NE:
      if (mScene.forest.bffX > 196 || mScene.forest.bffY < 60) { RandomizeBff(); }
        break;
      case E:
      if (mScene.forest.bffX > 196) { RandomizeBff(); }
        break;
      case SE:
      if (mScene.forest.bffX > 196 || mScene.forest.bffY > 196) { RandomizeBff(); }
        break;
    }
  }
  // end h4cky section

  // item hover
  if (Room()->itemId) {
    mIdleHoverIndex = (mIdleHoverIndex + 1) % HOVER_COUNT;
    Vec2 p = 16 * Room()->Data()->LocalCenter();
    MoveSprite(GetCube(), ITEM_SPRITE_ID, p.x-8, p.y + sHoverTable[mIdleHoverIndex]);
  }

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

void GameView::RandomizeBff() {
  using namespace BffDir;
  mScene.forest.bffDir = (uint8_t) Rand(8);
  switch(mScene.forest.bffDir) {
    case S:
      mScene.forest.bffX = 64 + (uint8_t) Rand(128);
      mScene.forest.bffY = (uint8_t) Rand(64);
      break;
    case SE:
      mScene.forest.bffX = (uint8_t) Rand(64);
      mScene.forest.bffY = (uint8_t) Rand(64);
      break;
    case E:
      mScene.forest.bffX = (uint8_t) Rand(64);
      mScene.forest.bffY = 64 + (uint8_t) Rand(128);
      break;
    case NE:
      mScene.forest.bffX = (uint8_t) Rand(64);
      mScene.forest.bffY = 192 + (uint8_t) Rand(64);
      break;
    case N:
      mScene.forest.bffX = 64 + (uint8_t) Rand(128);
      mScene.forest.bffY = 192 + (uint8_t) Rand(64);
      break;
    case NW:
      mScene.forest.bffX = 192 + (uint8_t) Rand(64);
      mScene.forest.bffY = 192 + (uint8_t) Rand(64);
      break;
    case W:
      mScene.forest.bffX = 192 + (uint8_t) Rand(64);
      mScene.forest.bffY = 64 + (uint8_t) Rand(128);
      break;
    case SW:
      mScene.forest.bffX = 192 + (uint8_t) Rand(128);
      mScene.forest.bffY = (uint8_t) Rand(64);
      break;
  }
}

bool GameView::ShowLocation(Vec2 room) {
  if (room == mRoom) { return false; }
  mRoom = room;
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
    mIdleHoverIndex = 0;

    // h4cky scene-specific stuff
    if (pGame->map.Data() == &dungeon_data) {
      mScene.dungeon.torchTime = 0;
    } else if (pGame->map.Data() == &forest_data) {
      if (mr->itemId) {
        mScene.forest.hasBff = 0;
      } else if ( (mScene.forest.hasBff = (Rand(3) == 0)) ) {
        RandomizeBff();
        ResizeSprite(GetCube(), BFF_SPRITE_ID, 8, 8);
        SetSpriteImage(GetCube(), BFF_SPRITE_ID, Butterfly.index + 4 * mScene.forest.bffDir);
        MoveSprite(GetCube(), BFF_SPRITE_ID, mScene.forest.bffX-68, mScene.forest.bffY-68);
      }
    }
    // end h4cky stuff

  } else {
    HideRoom();
  }
  pGame->NeedsSync();
  return true;
}

bool GameView::HideRoom() {
  bool result = mRoom != ROOM_NONE;
  mRoom = ROOM_NONE;
  // hide sprites
  HideSprite(GetCube(), PLAYER_SPRITE_ID);
  HideSprite(GetCube(), ITEM_SPRITE_ID);
  HideSprite(GetCube(), BFF_SPRITE_ID);
  if(result) {
    mScene.idle.time = 0;
  }

  DrawInventorySprites();
  DrawBackground();
  return result;
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
  HideSprite(GetCube(), PLAYER_SPRITE_ID);
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
  HideSprite(GetCube(), ITEM_SPRITE_ID);
}


void GameView::RefreshInventory() {
  if (!IsShowingRoom()) {
    mScene.idle.time = 0;
    DrawInventorySprites();
  }
}

void GameView::DrawInventorySprites() {
  mScene.idle.count = 0;
  const int pad = 24;
  const int innerPad = (128-pad-pad)/3;
  const int firstSandwichId = 2;
  const int sandwichTypeCount = 4;
  for(int itemId=firstSandwichId; itemId<firstSandwichId+sandwichTypeCount; ++itemId) {
    if (pGame->player.HasItem(itemId)) {
      ResizeSprite(GetCube(), mScene.idle.count, 16, 16);
      MoveSprite(GetCube(), mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      SetSpriteImage(GetCube(), mScene.idle.count, Items.index + (itemId-1) * Items.width * Items.height);
      mScene.idle.count++;
    }
  }
  if (pGame->player.HaveBasicKey()) {
      ResizeSprite(GetCube(), mScene.idle.count, 16, 16);
      MoveSprite(GetCube(), mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      SetSpriteImage(GetCube(), mScene.idle.count, Items.index + (ITEM_BASIC_KEY-1) * Items.width * Items.height);
      mScene.idle.count++;
  }
  for(int i=mScene.idle.count; i<4; ++i) {
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

#ifdef SIFTEO_SIMULATOR
#define TILT_THRESHOLD 50
#else
#define TILT_THRESHOLD 35
#endif

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
  return neighbor == CUBE_ID_UNDEFINED ? 0 : pGame->views + (neighbor-CUBE_ID_BASE);
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
