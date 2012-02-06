#include "RoomView.h"
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

void RoomView::Init(unsigned roomId) {
  flags.hideOverlay = false;
  VidMode_BG0_SPR_BG1 mode(Parent()->GetCube()->vbuf);
  mode.set();
  mode.clear();
  mode.setWindow(0, 128);
  flags.hideOverlay = false;
  mRoomId = roomId;
  // are we showing an items?
  mStartFrame = pGame->AnimFrame();
  ComputeAnimatedTiles();
  Parent()->HideSprites();
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
  if (this == pGame->GetPlayer()->View()) { 
    ShowPlayer(); 
  }
  DrawBackground();
  // h4cky scene-specific stuff
  /*
  if (pGame->GetMap()->Data() == &forest_data) {
    if (mr->itemId) {
      mScene.forest.hasBff = 0;
    } else if ( (mScene.forest.hasBff = (gRandom.randrange(3) == 0)) ) {
      RandomizeBff();
      ResizeSprite(Parent()->GetCube(), BFF_SPRITE_ID, 8, 8);
      SetSpriteImage(Parent()->GetCube(), BFF_SPRITE_ID, Butterfly.index + 4 * mScene.forest.bffDir);
      MoveSprite(Parent()->GetCube(), BFF_SPRITE_ID, mScene.forest.bffX-68, mScene.forest.bffY-68);
    }
  }
  */
  // end h4cky stuff
  pGame->NeedsSync();
}

void RoomView::Restore() {
  Init(mRoomId);
}

void RoomView::Update() {
  VidMode_BG0_SPR_BG1 mode(Parent()->GetCube()->vbuf);
  // update animated tiles (could suffer some optimization)
  const unsigned t = pGame->AnimFrame() - mStartFrame;
  for(unsigned i=0; i<mAnimTileCount; ++i) {
    const AnimTile& view = mAnimTiles[i];
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
    MoveSprite(Parent()->GetCube(), BFF_SPRITE_ID, mScene.forest.bffX-68, mScene.forest.bffY-68);
    // hack - assumes butterflies and items are not rendered on same cube
    mIdleHoverIndex = (mIdleHoverIndex + 1) % (BFF_FRAME_COUNT * FRAMES_PER_TORCH_FRAME);
    SetSpriteImage(Parent()->GetCube(), BFF_SPRITE_ID, 
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
    const unsigned hoverTime = (pGame->AnimFrame() - mStartFrame) % HOVER_COUNT;
    Vec2 p = 16 * GetRoom()->LocalCenter(0);
    mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y + sHoverTable[hoverTime]);
  }
}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

Room* RoomView::GetRoom() const { 
  return pGame->GetMap()->GetRoom(mRoomId);
}

Vec2 RoomView::Location() const {
  return pGame->GetMap()->GetLocation(mRoomId);
}

/*
void RoomView::RandomizeBff() {
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

void RoomView::HideOverlay(bool flag) {
  if (flags.hideOverlay != flag) {
    flags.hideOverlay = flag;
    DrawBackground();
    pGame->NeedsSync();
  }
}

void RoomView::ShowPlayer() {
  VidMode_BG0_SPR_BG1(Parent()->GetCube()->vbuf).resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void RoomView::SetPlayerFrame(unsigned frame) {
  VidMode_BG0_SPR_BG1(Parent()->GetCube()->vbuf).setSpriteImage(PLAYER_SPRITE_ID, frame);
}

void RoomView::UpdatePlayer() {
  Vec2 localPosition = pGame->GetPlayer()->Position() - 128 * Location();
  VidMode_BG0_SPR_BG1 mode(Parent()->GetCube()->vbuf);
  mode.setSpriteImage(PLAYER_SPRITE_ID, pGame->GetPlayer()->AnimFrame());
  mode.moveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void RoomView::HidePlayer() {
  VidMode_BG0_SPR_BG1(Parent()->GetCube()->vbuf).hideSprite(PLAYER_SPRITE_ID);
}
  
void RoomView::SetItemPosition(Vec2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  VidMode_BG0_SPR_BG1(Parent()->GetCube()->vbuf).moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void RoomView::HideItem() {
  VidMode_BG0_SPR_BG1(Parent()->GetCube()->vbuf).hideSprite(TRIGGER_SPRITE_ID);
}


//----------------------------------------------------------------------
// HELPERS
//----------------------------------------------------------------------

void RoomView::DrawBackground() {
  VidMode_BG0 mode(Parent()->GetCube()->vbuf);
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
  BG1Helper ovrly(*(Parent()->GetCube()));
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

void RoomView::ComputeAnimatedTiles() {
  mAnimTileCount = 0;
  const unsigned tc = pGame->GetMap()->Data()->animatedTileCount;
  if (mRoomId == ROOM_UNDEFINED || tc == 0) { return; }
  const AnimatedTileData* pAnims = pGame->GetMap()->Data()->animatedTiles;
  for(unsigned lid=0; lid<64; ++lid) {
    uint8_t tid = pGame->GetMap()->Data()->rooms[mRoomId].tiles[lid];
    bool is_animated = false;
    for(unsigned i=0; i<tc; ++i) {
      if (pAnims[i].tileId == tid) {
        AnimTile& view = mAnimTiles[mAnimTileCount];
        view.lid = lid;
        view.frameCount = pAnims[i].frameCount;
        mAnimTileCount++;
        break;
      }
    }
    if (mAnimTileCount == ANIM_TILE_CAPACITY) { break; }
  }
}
