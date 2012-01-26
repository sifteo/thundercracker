#include "GameView.h"
#include "Game.h"

// Constants

#define LOCATION_UNDEFINED  (Vec2(-1,-1))
#define BFF_SPRITE_ID       0
#define TRIGGER_SPRITE_ID   1
#define PLAYER_SPRITE_ID    3

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
touched(false), visited(0), mRoom(LOCATION_UNDEFINED), mIdleHoverIndex(0) {
}

void GameView::Init() {
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mode.set();
  mode.clear();
  mode.setWindow(0, 128);
  if (pGame->player.CurrentView() == this) {
    mRoom = Vec2(-1,-1);
    ShowLocation(pGame->player.Location());
  } else {
    mRoom.x = -2; // h4ck
    ShowLocation(LOCATION_UNDEFINED);
  }
}

Cube* GameView::GetCube() const {
  int id = (int)(this - pGame->ViewBegin());
  return gCubes + id;
}

void GameView::Update() {
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  if (!IsShowingRoom()) { 
    // compute position of each inventory item based on phase and current time
    if (mScene.idle.count > 0 && mScene.idle.time <= HOP_PHASE * (mScene.idle.count-1) + HOP_COUNT) {
      int count = 0;
      const int pad = 24;
      const int innerPad = (128-pad-pad)/3;
      for(int i=0; i<mScene.idle.count; ++i) {
        int localTime = mScene.idle.time - HOP_PHASE * i;
        if (localTime < HOP_COUNT) {
          mode.moveSprite(i, pad + innerPad * i - 8, 108  - sHopTable[localTime]);
        } else {
          mode.moveSprite(i, pad + innerPad * i - 8, 108);
        }
      }
      mScene.idle.time++;
    }
    return; 
  }

  // begin h4cky scene-specific stuff
  /*
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
  */
  // end h4cky section

  // item hover
  if (CurrentRoom()->HasItem()) {
    mIdleHoverIndex = (mIdleHoverIndex + 1) % HOVER_COUNT;
    Vec2 p = 16 * CurrentRoom()->LocalCenter();
    mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y + sHoverTable[mIdleHoverIndex]);
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

Room* GameView::CurrentRoom() const { 
  ASSERT(IsShowingRoom()); 
  return pGame->map.GetRoom(mRoom); 
}

void GameView::RandomizeBff() {
  using namespace BffDir;
  mScene.forest.bffDir = (uint8_t) gRandom.randrange(8);
  switch(mScene.forest.bffDir) {
    case S:
      mScene.forest.bffX = 64 + (uint8_t) gRandom.randrange(128);
      mScene.forest.bffY = (uint8_t) gRandom.randrange(64);
      break;
    case SE:
      mScene.forest.bffX = (uint8_t) gRandom.randrange(64);
      mScene.forest.bffY = (uint8_t) gRandom.randrange(64);
      break;
    case E:
      mScene.forest.bffX = (uint8_t) gRandom.randrange(64);
      mScene.forest.bffY = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case NE:
      mScene.forest.bffX = (uint8_t) gRandom.randrange(64);
      mScene.forest.bffY = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case N:
      mScene.forest.bffX = 64 + (uint8_t) gRandom.randrange(128);
      mScene.forest.bffY = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case NW:
      mScene.forest.bffX = 192 + (uint8_t) gRandom.randrange(64);
      mScene.forest.bffY = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case W:
      mScene.forest.bffX = 192 + (uint8_t) gRandom.randrange(64);
      mScene.forest.bffY = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case SW:
      mScene.forest.bffX = 192 + (uint8_t) gRandom.randrange(128);
      mScene.forest.bffY = (uint8_t) gRandom.randrange(64);
      break;
  }
}

bool GameView::ShowLocation(Vec2 room) {
  if (room == mRoom) { return false; }
  mRoom = room;
  // are we showing an items?
  if (IsShowingRoom()) {
    VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
    HideInventorySprites();
    Room* r = CurrentRoom();
    switch(r->TriggerType()) {
      case TRIGGER_ITEM: 
        mode.setSpriteImage(TRIGGER_SPRITE_ID, Items.index + (r->TriggerAsItem()->itemId - 1) * Items.width * Items.height);
        mode.resizeSprite(TRIGGER_SPRITE_ID, 16, 16);
        {
          Vec2 p = 16 * CurrentRoom()->LocalCenter();
          mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
        }
        break;
      case TRIGGER_NPC:
        const NpcData* npc = r->TriggerAsNPC();
        const DialogData& dialog = gDialogData[npc->dialog];
        mode.setSpriteImage(TRIGGER_SPRITE_ID, dialog.npc->index);
        mode.resizeSprite(TRIGGER_SPRITE_ID, 32, 32);
        mode.moveSprite(TRIGGER_SPRITE_ID, npc->x-16, npc->y-16);
        break;
    }
    if (this == pGame->player.CurrentView()) { ShowPlayer(); }
    DrawBackground();
    mIdleHoverIndex = 0;

    // h4cky scene-specific stuff
    /*
    if (pGame->map.Data() == &dungeon_data) {
      mScene.dungeon.torchTime = 0;
    } else if (pGame->map.Data() == &forest_data) {
      if (mr->itemId) {
        mScene.forest.hasBff = 0;
      } else if ( (mScene.forest.hasBff = (gRandom.randrange(3) == 0)) ) {
        RandomizeBff();
        ResizeSprite(GetCube(), BFF_SPRITE_ID, 8, 8);
        SetSpriteImage(GetCube(), BFF_SPRITE_ID, Butterfly.index + 4 * mScene.forest.bffDir);
        MoveSprite(GetCube(), BFF_SPRITE_ID, mScene.forest.bffX-68, mScene.forest.bffY-68);
      }
    }
    */
    // end h4cky stuff

  } else {
    HideRoom();
  }
  pGame->NeedsSync();
  return true;
}

bool GameView::HideRoom() {
  bool result = mRoom != LOCATION_UNDEFINED;
  mRoom = LOCATION_UNDEFINED;
  // hide sprites
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mode.hideSprite(PLAYER_SPRITE_ID);
  mode.hideSprite(TRIGGER_SPRITE_ID);
  mode.hideSprite(BFF_SPRITE_ID);
  if(result) {
    mScene.idle.time = 0;
  }

  DrawInventorySprites();
  DrawBackground();
  return result;
}

void GameView::ShowPlayer() {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void GameView::SetPlayerFrame(unsigned frame) {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).setSpriteImage(PLAYER_SPRITE_ID, frame);
}

void GameView::UpdatePlayer() {
  Vec2 localPosition = pGame->player.Position() - 128 * mRoom;
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mode.setSpriteImage(PLAYER_SPRITE_ID, pGame->player.CurrentFrame());
  mode.moveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void GameView::HidePlayer() {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).hideSprite(PLAYER_SPRITE_ID);
}
  
void GameView::SetItemPosition(Vec2 p) {
  p += 16 * CurrentRoom()->LocalCenter();
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void GameView::HideItem() {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).hideSprite(TRIGGER_SPRITE_ID);
}

void GameView::RefreshInventory() {
  if (!IsShowingRoom()) {
    mScene.idle.time = 0;
    DrawInventorySprites();
  }
}

void GameView::DrawInventorySprites() {
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mScene.idle.count = 0;
  const int pad = 24;
  const int innerPad = (128-pad-pad)/3;
  const int firstSandwichId = 2;
  const int sandwichTypeCount = 4;
  for(int itemId=firstSandwichId; itemId<firstSandwichId+sandwichTypeCount; ++itemId) {
    if (pGame->player.HasItem(itemId)) {
      mode.resizeSprite(mScene.idle.count, 16, 16);
      mode.moveSprite(mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      mode.setSpriteImage(mScene.idle.count, Items.index + (itemId-1) * Items.width * Items.height);
      mScene.idle.count++;
    }
  }
  if (pGame->player.HaveBasicKey()) {
      mode.resizeSprite(mScene.idle.count, 16, 16);
      mode.moveSprite(mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      mode.setSpriteImage(mScene.idle.count, Items.index + (ITEM_BASIC_KEY-1) * Items.width * Items.height);
      mScene.idle.count++;
  }
  for(int i=mScene.idle.count; i<4; ++i) {
    mode.hideSprite(i);
  }
}

void GameView::HideInventorySprites() {
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  for(int i=0; i<4; ++i) {
    mode.hideSprite(i);
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
          pGame->map.GetTileId(mRoom, Vec2(x, y))
        );
      }
    }
    // hack alert!
    if (CurrentRoom()->HasOpenDoor()) {
      for(int y=0; y<3; ++y) {
        for(int x=3; x<=4; ++x) {
          mode.BG0_drawAsset(
            Vec2(x<<1,y<<1),
            *(pGame->map.Data()->tileset),
            pGame->map.GetTileId(mRoom, Vec2(x, y))+2
          );
        }
      }
    }


    BG1Helper ovrly(*GetCube());
    const uint8_t *p = CurrentRoom()->Data()->overlay;
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
