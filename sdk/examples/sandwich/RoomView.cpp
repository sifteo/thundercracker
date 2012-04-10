#include "RoomView.h"
#include "Game.h"
#include "DrawingHelpers.h"
#include "MapHelpers.h"

#define ROOM_UNDEFINED      0xff
#define BFF_SPRITE_ID       0
#define TRIGGER_SPRITE_ID   1
#define EQUIP_SPRITE_ID     2
#define PLAYER_SPRITE_ID    3
#define BLOCK_SPRITE_ID     4

void RoomView::Init(unsigned roomId) {
  flags.locked = false;
  mRoomId = roomId;
  Restore();
}

void RoomView::Lock() {
  flags.locked = true;
  gGame.OnViewLocked(this);
}

void RoomView::Unlock() {
  flags.locked = false;
  gGame.OnViewUnlocked(this);
}

void RoomView::HideOverlay() {
  BG1Helper(*Parent()->GetCube()).Flush();
}


void RoomView::Restore() {
  mWobbles = -1.f;
  Parent()->HideSprites();
  ViewMode mode = Parent()->Graphics();
  Map& map = *gGame.GetMap();
  flags.hideOverlay = false;
  // are we showing an items?
  mStartFrame = gGame.AnimFrame();
  ComputeAnimatedTiles();
  Room* r = GetRoom();
  if (r->HasItem()) {
    ShowItem(r->Item());
  }
  if (this->Parent() == gGame.GetPlayer()->View()) { 
    ShowPlayer(); 
  }
  DrawBackground();
  // initialize ambient fx?
  if (map.Data()->ambientType) {
    if (r->HasTrigger()) {
      mAmbient.bff.active = 0;
    } else if ( (mAmbient.bff.active = (gRandom.randrange(3) == 0)) ) {
      mAmbient.bff.Randomize();
      mode.resizeSprite(BFF_SPRITE_ID, 8, 8);
      mode.setSpriteImage(BFF_SPRITE_ID, Butterfly.index + 4 * mAmbient.bff.dir);
      mode.moveSprite(BFF_SPRITE_ID, mAmbient.bff.pos.x-68, mAmbient.bff.pos.y-68);
    }
  }

  mBlock = 0;
  if (!r->IsSubdivided()) {
    // Should there be some sort of spatial hash?
    for(Sokoblock* pBlock = map.BlockBegin(); pBlock != map.BlockEnd(); ++pBlock) {
      if (GetRoom()->IsShowingBlock(pBlock)) {
        // assuming for now that we're only showing one block per cube :/
        ShowBlock(pBlock);
        break;
      }
    }
  }
}

void RoomView::Update() {
  ViewMode mode = Parent()->Graphics();
  // update animated tiles (could suffer some optimization)
  const unsigned t = gGame.AnimFrame() - mStartFrame;
  for(unsigned i=0; i<flags.animTileCount; ++i) {
    const AnimTile& view = mAnimTiles[i];
    const unsigned localt = t % (view.frameCount << 2);
    if (localt % 4 == 0) {
        mode.BG0_drawAsset(
          vec((view.lid%8)<<1,(view.lid>>3)<<1),
          *(gGame.GetMap()->Data()->tileset),
          gGame.GetMap()->Data()->roomTiles[mRoomId].tiles[view.lid] + (localt>>2)
        );
    }
  }

  if (gGame.GetMap()->Data()->ambientType && mAmbient.bff.active) {
    mAmbient.bff.Update();
    mode.moveSprite(BFF_SPRITE_ID, mAmbient.bff.pos.x-68, mAmbient.bff.pos.y-68);
    mode.setSpriteImage(
      BFF_SPRITE_ID, 
      Butterfly.index + 4 * mAmbient.bff.dir + mAmbient.bff.frame / 3
    );
  }

  // item hover
  if (GetRoom()->HasItem()) {
    const unsigned hoverTime = (gGame.AnimFrame() - mStartFrame) % HOVER_COUNT;
    Int2 p = 16 * GetRoom()->LocalCenter(0);
    mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y + kHoverTable[hoverTime]);
  }

  // nod or shake
  if (IsWobbly()) {
    mWobbles = clamp(mWobbles- 2.f*gGame.Dt().seconds(), 0.f, 1.f); // duration = 0.5
    ASSERT(flags.wobbleType != WOBBLE_UNUSED_0);
    ASSERT(flags.wobbleType != WOBBLE_UNUSED_1);
    switch(flags.wobbleType) {
      
      case WOBBLE_NOD: {
        const float u = 8.f * -1.2f * mWobbles * sin(M_PI * mWobbles * mWobbles * mWobbles);
        mode.BG0_setPanning(vec(0.f, u));
        mode.BG1_setPanning(vec(0.f, u));
        break;
      }

      case WOBBLE_SHAKE: {
        const float u = 8.f * 1.1f * mWobbles * sin(5 * M_PI * mWobbles);
        mode.BG0_setPanning(vec(u, 0.f));
        mode.BG1_setPanning(vec(u, 0.f));
        break;
      }

      default: {
        const Cube::Side dir = flags.wobbleType - WOBBLE_SLIDE_TOP;
        const float u = 8.f * mWobbles * mWobbles * mWobbles;
        const Int2 pan = (u * kSideToUnit[(dir+2)%4]).toInt();
        mode.BG0_setPanning(pan);
        mode.BG1_setPanning(pan);
      }

    }
  }
}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

Room* RoomView::GetRoom() const { 
  return gGame.GetMap()->GetRoom(mRoomId);
}

Int2 RoomView::Location() const {
  return gGame.GetMap()->GetLocation(mRoomId);
}

bool RoomView::GatewayTouched() const {
  const Room* pRoom = GetRoom();
  if (pRoom->HasGateway()) {
    for(Cube::Side s=0; s<4; ++s) {
      const ViewSlot *view = Parent()->VirtualNeighborAt(s);
      return view && view->Touched() && view->ShowingGatewayEdge();
    }
  }
  return false;
}


void RoomView::HideOverlay(bool flag) {
  if (flags.hideOverlay != flag) {
    flags.hideOverlay = flag;
    DrawBackground();
    gGame.NeedsSync();
  }
}

void RoomView::StartNod() {
  // fill in extra rows (with real data?)
  ViewMode mode = Parent()->Graphics();
  for(int i=0; i<16; ++i) {
    mode.BG0_drawAsset(vec(i, 16), BlackTile);
    mode.BG0_drawAsset(vec(i, 17), BlackTile);
  }
  mWobbles = 1.f;
  flags.wobbleType = WOBBLE_NOD;
}

void RoomView::StartShake() {
  // fill in extra columns (with real data?)
  ViewMode mode = Parent()->Graphics();
  for(int i=0; i<16; ++i) {
    mode.BG0_drawAsset(vec(16, i), BlackTile);
    mode.BG0_drawAsset(vec(17, i), BlackTile);
  }
  mWobbles = 1.f;
  flags.wobbleType = WOBBLE_SHAKE;
}

void RoomView::StartSlide(Cube::Side side) {
  ASSERT(0 <= side && side < 4);
  ViewMode mode = Parent()->Graphics();
  if (side % 2 == 0) {
    for(int i=0; i<16; ++i) {
      mode.BG0_drawAsset(vec(i, 16), BlackTile);
      mode.BG0_drawAsset(vec(i, 17), BlackTile);
    }
  } else {
    for(int i=0; i<16; ++i) {
      mode.BG0_drawAsset(vec(16, i), BlackTile);
      mode.BG0_drawAsset(vec(17, i), BlackTile);
    }
  }
  mWobbles = 1.f;
  flags.wobbleType = WOBBLE_SLIDE_TOP + side;

}

//---------------------------------------------------------------
// SPRITE METHODS
//---------------------------------------------------------------

void RoomView::ShowPlayer() {
  ViewMode gfx = Parent()->Graphics();
  gfx.resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  if (gGame.GetPlayer()->Equipment()) {
    gfx.setSpriteImage(EQUIP_SPRITE_ID, Items, gGame.GetPlayer()->Equipment()->itemId);
  }
  UpdatePlayer();
}

void RoomView::ShowItem(const ItemData* item) {
  Room* pRoom = GetRoom();
  ViewMode mode = Parent()->Graphics();
  mode.setSpriteImage(TRIGGER_SPRITE_ID, Items, item->itemId);
  Int2 p = pRoom->HasDepot() ? 
    16 * vec(pRoom->Depot()->tx+1, pRoom->Depot()->ty+1) : 
    16 * pRoom->LocalCenter(0);
  mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void RoomView::ShowBlock(Sokoblock *pBlock) {
  mBlock = pBlock;
  Parent()->Graphics().setSpriteImage(BLOCK_SPRITE_ID, pBlock->Asset());
  UpdateBlock();
}

void RoomView::SetPlayerFrame(unsigned frame) {
  Parent()->Graphics().setSpriteImage(PLAYER_SPRITE_ID, frame);
}

void RoomView::SetEquipPosition(Int2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  ViewMode gfx = Parent()->Graphics();
  gfx.setSpriteImage(EQUIP_SPRITE_ID, Items, gGame.GetPlayer()->Equipment()->itemId);
  gfx.moveSprite(EQUIP_SPRITE_ID, p.x-8, p.y);
}
  
void RoomView::SetItemPosition(Int2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  Parent()->Graphics().moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void RoomView::UpdatePlayer() {
  Int2 localPosition = gGame.GetPlayer()->Position() - 128 * Location();
  ViewMode gfx = Parent()->Graphics();
  gfx.setSpriteImage(PLAYER_SPRITE_ID, gGame.GetPlayer()->AnimFrame());
  gfx.moveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
  if (gGame.GetPlayer()->Equipment()) {
    gfx.moveSprite(EQUIP_SPRITE_ID, localPosition.x-8, localPosition.y-ITEM_OFFSET);
  }
}

void RoomView::DrawPlayerFalling(int height) {
  ViewMode mode = Parent()->Graphics();
  Int2 localCenter = 16 * GetRoom()->LocalCenter(0);
  mode.setSpriteImage(PLAYER_SPRITE_ID, PlayerStand.index + (2<<4));
  mode.moveSprite(PLAYER_SPRITE_ID, localCenter.x-16, localCenter.y-32-height);
  mode.resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  if (gGame.GetPlayer()->Equipment()) { 
    mode.moveSprite(EQUIP_SPRITE_ID, localCenter.x-8, localCenter.y-16-height-ITEM_OFFSET);
  }
}

void RoomView::UpdateBlock() {
  ASSERT(mBlock);
  const Int2 localPosition = mBlock->Position() - vec(32, 32) - 128 * Location();
  Parent()->Graphics().moveSprite(BLOCK_SPRITE_ID, localPosition);
}

void RoomView::HidePlayer() {
  ViewMode gfx = Parent()->Graphics();
  gfx.hideSprite(PLAYER_SPRITE_ID);
  gfx.hideSprite(EQUIP_SPRITE_ID);
}

void RoomView::HideItem() { Parent()->Graphics().hideSprite(TRIGGER_SPRITE_ID); }
void RoomView::HideEquip() { Parent()->Graphics().hideSprite(EQUIP_SPRITE_ID); }
void RoomView::HideBlock() { 
  Parent()->Graphics().hideSprite(BLOCK_SPRITE_ID); 
  mBlock = 0;
}

//----------------------------------------------------------------------
// TRAPDOOR METHODS
//----------------------------------------------------------------------

void RoomView::DrawTrapdoorFrame(int frame) {
  ViewMode mode = Parent()->Graphics();
  Int2 firstTile = GetRoom()->LocalCenter(0) - vec(2,2);
  for(unsigned y=0; y<4; ++y)
  for(unsigned x=0; x<4; ++x) {
    mode.BG0_drawAsset(
      vec(firstTile.x + x, firstTile.y + y) << 1,
      *(gGame.GetMap()->Data()->tileset),
      gGame.GetMap()->GetTileId(mRoomId, vec(firstTile.x + x, firstTile.y + y))+frame
    );    
  }
}

//----------------------------------------------------------------------
// HELPERS
//----------------------------------------------------------------------

void RoomView::RefreshDoor() {
  ViewMode g = Parent()->Graphics();
  const Room *pRoom = GetRoom();
  if (pRoom->HasOpenDoor()) {
    for(int y=0; y<3; ++y)
    for(int x=3; x<5; ++x) {
      g.BG0_drawAsset(
        vec(x,y) << 1,
        *(gGame.GetMap()->Data()->tileset),
        gGame.GetMap()->GetTileId(mRoomId, vec(x, y))+2
      );
    }
  }
}

void RoomView::RefreshDepot() {
  ViewMode g = Parent()->Graphics();
  const Room *pRoom = GetRoom();
  if (pRoom->HasDepotContents()) {
    const DepotData& depot = *pRoom->Depot();
    // assuming depots are door sizes (2x3)
    for(int y=depot.ty; y<depot.ty+3; ++y)
    for(int x=depot.tx; x<depot.tx+2; ++x) {
      g.BG0_drawAsset(
        vec(x,y) << 1,
        *(gGame.GetMap()->Data()->tileset),
        gGame.GetMap()->GetTileId(mRoomId, vec(x, y))+2
      );
    }
    ShowItem(pRoom->DepotContents());
  }

}

void RoomView::ShowFrame() {
  ViewMode g = Parent()->Graphics();
  g.BG0_drawAsset(Vec2(0,0), FrameTop);
  g.BG0_drawAsset(Vec2(0, 1), FrameLeft);
  g.BG0_drawAsset(Vec2(15, 1), FrameRight);
  g.BG0_drawAsset(Vec2(0, 15), FrameBottom);
  HideOverlay();
}

void RoomView::DrawBackground() {
  ViewMode mode = Parent()->Graphics();
  mode.BG0_setPanning(vec(0,0));
  DrawRoom(&mode, gGame.GetMap()->Data(), mRoomId);
  RefreshDoor();
  RefreshDepot();

  const Room *pRoom = GetRoom();

  BG1Helper ovrly(*(Parent()->GetCube()));
  if (!flags.hideOverlay && pRoom->HasOverlay()) {
    DrawRoomOverlay(&ovrly, gGame.GetMap()->Data(), pRoom->OverlayTile(), pRoom->OverlayBegin());
  }
  if (pRoom->HasNPC()) {
      const NpcData* npc = pRoom->NPC();
      const DialogData& dialog = gDialogData[npc->dialog];
      ovrly.DrawAsset(vec((npc->x-16)>>3, (npc->y-16)>>3), *dialog.npc);
  }

  ovrly.Flush();
}

void RoomView::ComputeAnimatedTiles() {
  flags.animTileCount = 0;
  const unsigned tc = gGame.GetMap()->Data()->animatedTileCount;
  if (mRoomId == ROOM_UNDEFINED || tc == 0) { return; }
  const AnimatedTileData* pAnims = gGame.GetMap()->Data()->animatedTiles;
  for(unsigned lid=0; lid<64; ++lid) {
    uint8_t tid = gGame.GetMap()->Data()->roomTiles[mRoomId].tiles[lid];
    bool is_animated = false;
    for(unsigned i=0; i<tc; ++i) {
      if (pAnims[i].tileId == tid) {
        AnimTile& view = mAnimTiles[flags.animTileCount];
        view.lid = lid;
        view.frameCount = pAnims[i].frameCount;
        flags.animTileCount++;
        break;
      }
    }
    if (flags.animTileCount == ANIM_TILE_CAPACITY) { break; }
  }
}
