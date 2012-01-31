#include "GameView.h"
#include "Game.h"

// Constants

#define ROOM_UNDEFINED  (0xff)
#define BFF_SPRITE_ID       0
#define TRIGGER_SPRITE_ID   1
#define PLAYER_SPRITE_ID    2

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

#define BFF_FRAME_COUNT 4

/*
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
*/

// methods

void GameView::Init() {
  flags.hideOverlay = false;
  flags.prevTouch = GetCube()->touching();
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mode.set();
  mode.clear();
  mode.setWindow(0, 128);
  if (pGame->GetPlayer()->View() == this) {
    mRoomId = ROOM_UNDEFINED;
    ShowLocation(pGame->GetPlayer()->Location());
  } else {
    mRoomId = 0;
    ShowLocation(Vec2(-1,-1));
  }
}

Cube::ID GameView::GetCubeID() const {
  return this - pGame->ViewBegin();
}

Cube* GameView::GetCube() const {
  return gCubes + (this - pGame->ViewBegin());
}

void GameView::Update() {
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  if (!IsShowingRoom()) { 
    // compute position of each inventory item based on phase and current time
    const unsigned t = pGame->AnimFrame() - mScene.idle.startFrame;
    if (mScene.idle.count > 0 && t <= HOP_PHASE * (mScene.idle.count-1) + HOP_COUNT) {
      int count = 0;
      const int pad = 24;
      const int innerPad = (128-pad-pad)/3;
      for(unsigned i=0; i<mScene.idle.count; ++i) {
        int localTime = t - HOP_PHASE * i;
        if (localTime < HOP_COUNT) {
          mode.moveSprite(i, pad + innerPad * i - 8, 108  - sHopTable[localTime]);
        } else {
          mode.moveSprite(i, pad + innerPad * i - 8, 108);
        }
      }
    }
    return; 
  }

  // update animated tiles (could suffer some optimization)
  const unsigned t = pGame->AnimFrame() - mScene.room.startFrame;
  for(unsigned i=0; i<mScene.room.animTileCount; ++i) {
    const AnimTileView& view = mScene.room.animTiles[i];
    const unsigned localt = t % (view.frameCount << 2);
    if (localt % 4 == 0) {
        mode.BG0_drawAsset(
          Vec2((view.lid%8)<<1,(view.lid>>3)<<1),
          *(pGame->GetMap()->Data()->tileset),
          pGame->GetMap()->Data()->rooms[mRoomId].tiles[view.lid] + (localt>>2)
        );
    }
  }

  // begin h4cky scene-specific stuff
  /*
  const RoomData *p = Room()->Data();
  if (pGame->GetMap()->Data() == &forest_data && mScene.forest.hasBff) {
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
  if (GetRoom()->HasItem()) {
    const unsigned hoverTime = (pGame->AnimFrame() - mScene.room.startFrame) % HOVER_COUNT;
    Vec2 p = 16 * GetRoom()->LocalCenter(0);
    mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y + sHoverTable[hoverTime]);
  }

}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

Vec2 GameView::Location() const {
  return pGame->GetMap()->GetLocation(mRoomId);
}

bool GameView::IsShowingRoom() const {
  return mRoomId != ROOM_UNDEFINED;
}

Room* GameView::GetRoom() const { 
  ASSERT(IsShowingRoom()); 
  return pGame->GetMap()->GetRoom(mRoomId);
}
/*
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
*/

bool GameView::ShowLocation(Vec2 room) {
  const unsigned roomId = pGame->GetMap()->Contains(room) ? pGame->GetMap()->GetRoomId(room) : ROOM_UNDEFINED;
  if (roomId == mRoomId) { return false; }
  flags.hideOverlay = false;
  mRoomId = roomId;
  // are we showing an items?
  if (IsShowingRoom()) {
    mScene.room.startFrame = pGame->AnimFrame();
    ComputeAnimatedTiles();
    VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
    HideInventorySprites();
    Room* r = GetRoom();
    switch(r->TriggerType()) {
      case TRIGGER_ITEM: 
        mode.setSpriteImage(TRIGGER_SPRITE_ID, Items.index + (r->TriggerAsItem()->itemId - 1) * Items.width * Items.height);
        mode.resizeSprite(TRIGGER_SPRITE_ID, 16, 16);
        {
          Vec2 p = 16 * GetRoom()->LocalCenter(0);
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
    if (this == pGame->GetPlayer()->View()) { ShowPlayer(); }
    DrawBackground();

    // h4cky scene-specific stuff
    /*
    if (pGame->GetMap()->Data() == &forest_data) {
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
  bool result = mRoomId != ROOM_UNDEFINED;
  flags.hideOverlay = false;
  mRoomId = ROOM_UNDEFINED;
  // hide sprites
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mode.hideSprite(PLAYER_SPRITE_ID);
  mode.hideSprite(TRIGGER_SPRITE_ID);
  mode.hideSprite(BFF_SPRITE_ID);
  if(result) {
    mScene.idle.startFrame = pGame->AnimFrame();
  }
  DrawInventorySprites();
  DrawBackground();
  return result;
}

void GameView::HideOverlay(bool flag) {
  if (flags.hideOverlay != flag) {
    flags.hideOverlay = flag;
    DrawBackground();
    pGame->NeedsSync();
  }
}

void GameView::ShowPlayer() {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void GameView::SetPlayerFrame(unsigned frame) {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).setSpriteImage(PLAYER_SPRITE_ID, frame);
}

void GameView::UpdatePlayer() {
  Vec2 localPosition = pGame->GetPlayer()->Position() - 128 * Location();
  VidMode_BG0_SPR_BG1 mode(GetCube()->vbuf);
  mode.setSpriteImage(PLAYER_SPRITE_ID, pGame->GetPlayer()->AnimFrame());
  mode.moveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void GameView::HidePlayer() {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).hideSprite(PLAYER_SPRITE_ID);
}
  
void GameView::SetItemPosition(Vec2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void GameView::HideItem() {
  VidMode_BG0_SPR_BG1(GetCube()->vbuf).hideSprite(TRIGGER_SPRITE_ID);
}

void GameView::RefreshInventory() {
  if (!IsShowingRoom()) {
    mScene.idle.startFrame = pGame->AnimFrame();
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
    if (pGame->GetPlayer()->HasItem(itemId)) {
      mode.resizeSprite(mScene.idle.count, 16, 16);
      mode.moveSprite(mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      mode.setSpriteImage(mScene.idle.count, Items.index + (itemId-1) * Items.width * Items.height);
      mScene.idle.count++;
    }
  }
  if (pGame->GetPlayer()->HaveBasicKey()) {
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
  return neighbor == CUBE_ID_UNDEFINED ? 0 : pGame->ViewAt(neighbor-CUBE_ID_BASE);
}

void GameView::DrawBackground() {
  VidMode_BG0 mode(GetCube()->vbuf);
  if (!IsShowingRoom()) {
    mode.BG0_drawAsset(Vec2(0,0), *(pGame->GetMap()->Data()->blankImage));
    BG1Helper(*GetCube()).Flush();
  } else {
    
    const Room *pRoom = GetRoom();
    for(int y=0; y<8; ++y) {
      for(int x=0; x<8; ++x) {
        mode.BG0_drawAsset(
          Vec2(x<<1,y<<1),
          *(pGame->GetMap()->Data()->tileset),
          pGame->GetMap()->GetTileId(mRoomId, Vec2(x, y))
        );
      }
    }
    // hack alert!
    if (pRoom->HasOpenDoor()) {
      for(int y=0; y<3; ++y) {
        for(int x=3; x<=4; ++x) {
          mode.BG0_drawAsset(
            Vec2(x<<1,y<<1),
            *(pGame->GetMap()->Data()->tileset),
            pGame->GetMap()->GetTileId(mRoomId, Vec2(x, y))+2
          );
        }
      }
    }

    BG1Helper ovrly(*GetCube());
    if (!flags.hideOverlay && pRoom->HasOverlay()) {
      unsigned tid = pRoom->OverlayTile();
      const uint8_t *pRle = pRoom->OverlayBegin();
      while(tid < 64) {
        if (*pRle == 0xff) {
          tid += pRle[1];
          pRle+=2;
        } else {
          ovrly.DrawAsset(2*Vec2(tid%8, tid>>3), *(pGame->GetMap()->Data()->overlay), *pRle);
          tid++;
          pRle++;
        }
      }
    }
    ovrly.Flush();
  }
}

void GameView::ComputeAnimatedTiles() {
  mScene.room.animTileCount = 0;
  const unsigned tc = pGame->GetMap()->Data()->animatedTileCount;
  if (mRoomId == ROOM_UNDEFINED || tc == 0) { return; }
  const AnimatedTileData* pAnims = pGame->GetMap()->Data()->animatedTiles;
  for(unsigned lid=0; lid<64; ++lid) {
    uint8_t tid = pGame->GetMap()->Data()->rooms[mRoomId].tiles[lid];
    bool is_animated = false;
    for(unsigned i=0; i<tc; ++i) {
      if (pAnims[i].tileId == tid) {
        AnimTileView& view = mScene.room.animTiles[mScene.room.animTileCount];
        view.lid = lid;
        view.frameCount = pAnims[i].frameCount;
        mScene.room.animTileCount++;
        break;
      }
    }
    if (mScene.room.animTileCount == ANIM_TILE_CAPACITY) { break; }
  }
}
