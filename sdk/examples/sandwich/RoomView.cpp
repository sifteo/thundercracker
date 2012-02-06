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

void RoomView::Init(unsigned roomId) {
  flags.hideOverlay = false;
  ViewMode mode = Parent()->Graphics();
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
  // initialize ambient fx?
  if (pGame->GetMap()->Data()->ambientType) {
    if (r->HasItem()) {
      mAmbient.bff.active = 0;
    } else if ( (mAmbient.bff.active = (gRandom.randrange(3) == 0)) ) {
      RandomizeBff();
      mode.resizeSprite(BFF_SPRITE_ID, 8, 8);
      mode.setSpriteImage(BFF_SPRITE_ID, Butterfly.index + 4 * mAmbient.bff.dir);
      mode.moveSprite(BFF_SPRITE_ID, mAmbient.bff.x-68, mAmbient.bff.y-68);
    }
  }
  pGame->NeedsSync();
}

void RoomView::Restore() {
  Init(mRoomId);
}

void RoomView::Update() {
  ViewMode mode = Parent()->Graphics();
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

  if (pGame->GetMap()->Data()->ambientType && mAmbient.bff.active) {
    // butterfly stuff
    Vec2 delta = sBffTable[mAmbient.bff.dir];
    mAmbient.bff.x += (uint8_t) delta.x;
    mAmbient.bff.y += (uint8_t) delta.y;
    mode.moveSprite(BFF_SPRITE_ID, mAmbient.bff.x-68, mAmbient.bff.y-68);
    // hack - assumes butterflies and items are not rendered on same cube
    mAmbient.bff.frame = (mAmbient.bff.frame + 1) % (BFF_FRAME_COUNT * 3);
    mode.setSpriteImage(
      BFF_SPRITE_ID, 
      Butterfly.index + 4 * mAmbient.bff.dir + mAmbient.bff.frame / 3
    );
    using namespace BffDir;
    switch(mAmbient.bff.dir) {
      case S:
        if (mAmbient.bff.y > 196) { RandomizeBff(); }
        break;
      case SW:
        if (mAmbient.bff.x < 60 || mAmbient.bff.y > 196) { RandomizeBff(); }
        break;
      case W:
        if (mAmbient.bff.x < 60) { RandomizeBff(); }
        break;
      case NW:
        if (mAmbient.bff.x < 60 || mAmbient.bff.y < 60) { RandomizeBff(); }
        break;
      case N:
      if (mAmbient.bff.y < 60) { RandomizeBff(); }
        break;
      case NE:
      if (mAmbient.bff.x > 196 || mAmbient.bff.y < 60) { RandomizeBff(); }
        break;
      case E:
      if (mAmbient.bff.x > 196) { RandomizeBff(); }
        break;
      case SE:
      if (mAmbient.bff.x > 196 || mAmbient.bff.y > 196) { RandomizeBff(); }
        break;
    }
  }

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

void RoomView::RandomizeBff() {
  using namespace BffDir;
  mAmbient.bff.dir = (uint8_t) gRandom.randrange(8);
  switch(mAmbient.bff.dir) {
    case S:
      mAmbient.bff.x = 64 + (uint8_t) gRandom.randrange(128);
      mAmbient.bff.y = (uint8_t) gRandom.randrange(64);
      break;
    case SE:
      mAmbient.bff.x = (uint8_t) gRandom.randrange(64);
      mAmbient.bff.y = (uint8_t) gRandom.randrange(64);
      break;
    case E:
      mAmbient.bff.x = (uint8_t) gRandom.randrange(64);
      mAmbient.bff.y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case NE:
      mAmbient.bff.x = (uint8_t) gRandom.randrange(64);
      mAmbient.bff.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case N:
      mAmbient.bff.x = 64 + (uint8_t) gRandom.randrange(128);
      mAmbient.bff.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case NW:
      mAmbient.bff.x = 192 + (uint8_t) gRandom.randrange(64);
      mAmbient.bff.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case W:
      mAmbient.bff.x = 192 + (uint8_t) gRandom.randrange(64);
      mAmbient.bff.y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case SW:
      mAmbient.bff.x = 192 + (uint8_t) gRandom.randrange(128);
      mAmbient.bff.y = (uint8_t) gRandom.randrange(64);
      break;
  }
}

void RoomView::HideOverlay(bool flag) {
  if (flags.hideOverlay != flag) {
    flags.hideOverlay = flag;
    DrawBackground();
    pGame->NeedsSync();
  }
}

void RoomView::ShowPlayer() {
  Parent()->Graphics().resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  UpdatePlayer();
}

void RoomView::SetPlayerFrame(unsigned frame) {
  Parent()->Graphics().setSpriteImage(PLAYER_SPRITE_ID, frame);
}

void RoomView::UpdatePlayer() {
  Vec2 localPosition = pGame->GetPlayer()->Position() - 128 * Location();
  ViewMode mode = Parent()->Graphics();
  mode.setSpriteImage(PLAYER_SPRITE_ID, pGame->GetPlayer()->AnimFrame());
  mode.moveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
}

void RoomView::HidePlayer() {
  Parent()->Graphics().hideSprite(PLAYER_SPRITE_ID);
}
  
void RoomView::SetItemPosition(Vec2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  Parent()->Graphics().moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void RoomView::HideItem() {
  Parent()->Graphics().hideSprite(TRIGGER_SPRITE_ID);
}


//----------------------------------------------------------------------
// HELPERS
//----------------------------------------------------------------------

void RoomView::DrawBackground() {
  ViewMode mode = Parent()->Graphics();
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
