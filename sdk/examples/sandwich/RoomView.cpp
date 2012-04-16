#include "RoomView.h"
#include "Game.h"
#include "DrawingHelpers.h"
#include "MapHelpers.h"

#define ROOM_UNDEFINED      0xff
#define mCanvas         (Parent().Canvas())  
#define mBffSprite      (Parent().Canvas().sprites[0])
#define mTriggerSprite  (Parent().Canvas().sprites[1])
#define mEquipSprite    (Parent().Canvas().sprites[2])
#define mPlayerSprite   (Parent().Canvas().sprites[3])
#define mNpcSprite      (Parent().Canvas().sprites[4])
#define mBlockSprite    (Parent().Canvas().sprites[5])
  
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

void RoomView::ShowOverlay() {
  if (flags.hideOverlay) {
    flags.hideOverlay = false;
    RefreshOverlay();
  }
}

void RoomView::RefreshOverlay() {
  auto& room = GetRoom();
  if (room.HasOverlay() && !flags.hideOverlay) {
    Parent().DrawRoomOverlay(room.OverlayTile(), room.OverlayBegin());
    Parent().FlagOverlay();
  }
}

void RoomView::HideOverlay() {
  if (GetRoom().HasOverlay() && !flags.hideOverlay) {
    flags.hideOverlay = true;
    mCanvas.bg1.eraseMask();
    Parent().EnqueueHackyTouches();
    Parent().FlagOverlay(false);
  }
}


void RoomView::Restore() {
  mWobbles = -1.f;
  Map& map = gGame.GetMap();
  flags.hideOverlay = false;
  // are we showing an items?
  mStartFrame = gGame.AnimFrame();
  ComputeAnimatedTiles();
  auto& r = GetRoom();
  if (r.HasItem()) {
    ShowItem(&r.Item());
  }
  if (r.HasNPC()) {
    auto& npc = r.NPC();
    auto& dialog = gDialogData[npc.dialog];
    auto& img = *dialog.npc;
    mNpcSprite.setImage(img);
    mNpcSprite.move(vec(npc.x,npc.y) - img.pixelSize()/2);
  }
  if (&this->Parent() == &gGame.GetPlayer().View()) { 
    ShowPlayer(); 
  }
  mCanvas.bg0.erase(BlackTile);
  DrawBackground();
  // initialize ambient fx?
  if (map.Data().ambientType) {
    if (r.HasTrigger()) {
      mAmbient.bff.active = 0;
    } else if ( (mAmbient.bff.active = (gRandom.randrange(3) == 0)) ) {
      mAmbient.bff.Randomize();
      mBffSprite.setImage(Butterfly, 4 * mAmbient.bff.dir);
      mBffSprite.move(mAmbient.bff.pos.x-68, mAmbient.bff.pos.y-68);
    }
  }

  mBlock = 0;
  if (!r.IsSubdivided()) {
    // Should there be some sort of spatial hash?
    for(Sokoblock* pBlock = map.BlockBegin(); pBlock != map.BlockEnd(); ++pBlock) {
      if (r.IsShowingBlock(pBlock)) {
        // assuming for now that we're only showing one block per cube :/
        ShowBlock(pBlock);
        break;
      }
    }
  }
}

void RoomView::Update() {
  // update animated tiles (could suffer some optimization)
  const unsigned t = gGame.AnimFrame() - mStartFrame;


  for(unsigned i=0; i<flags.animTileCount; ++i) {
    const AnimTile& view = mAnimTiles[i];
    const unsigned localt = t % (view.frameCount << 2);
    if (localt % 4 == 0) {
        mCanvas.bg0.image(
          vec((view.lid%8)<<1,(view.lid>>3)<<1),
          *(gGame.GetMap().Data().tileset),
          gGame.GetMap().GetTileId(mRoomId, view.lid) + (localt>>2)
        );
    }
  }

  if (gGame.GetMap().Data().ambientType && mAmbient.bff.active) {
    mAmbient.bff.Update();
    mBffSprite.move(mAmbient.bff.pos.x-68, mAmbient.bff.pos.y-68);
    mBffSprite.setImage(Butterfly, 4 * mAmbient.bff.dir + mAmbient.bff.frame / 3);
  }

  // item hover
  if (GetRoom().HasItem()) {
    const unsigned hoverTime = (gGame.AnimFrame() - mStartFrame) % HOVER_COUNT;
    Int2 p = 16 * GetRoom().LocalCenter(0);
    mTriggerSprite.move(p.x-8, p.y + kHoverTable[hoverTime]);
  }

  // nod or shake
  if (IsWobbly()) {
    mWobbles = clamp(mWobbles- 2.f*gGame.Dt().seconds(), 0.f, 1.f); // duration = 0.5
    ASSERT(flags.wobbleType != WOBBLE_UNUSED_0);
    ASSERT(flags.wobbleType != WOBBLE_UNUSED_1);
    switch(flags.wobbleType) {
      
      case WOBBLE_NOD: {
        const float u = 8.f * -1.2f * mWobbles * sin(M_PI * mWobbles * mWobbles * mWobbles);
        mCanvas.bg0.setPanning(vec(0.f, u));
        mCanvas.bg1.setPanning(vec(0.f, u));
        break;
      }

      case WOBBLE_SHAKE: {
        const float u = 8.f * 1.1f * mWobbles * sin(5 * M_PI * mWobbles);
        mCanvas.bg0.setPanning(vec(u, 0.f));
        mCanvas.bg1.setPanning(vec(u, 0.f));
        break;
      }

      default: {
        const Side dir = (Side)(flags.wobbleType - WOBBLE_SLIDE_TOP);
        const float u = 8.f * mWobbles * mWobbles * mWobbles;
        const Int2 pan = (u * Int2::unit((dir+2)%4));
        mCanvas.bg0.setPanning(pan);
        mCanvas.bg1.setPanning(pan);
      }

    }
  }
}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

Room& RoomView::GetRoom() const { 
  return gGame.GetMap().GetRoom(mRoomId);
}

Int2 RoomView::Location() const {
  return gGame.GetMap().GetLocation(mRoomId);
}

void RoomView::StartNod() {
  // fill in extra rows (with real data?)
  mWobbles = 1.f;
  flags.wobbleType = WOBBLE_NOD;
}

void RoomView::StartShake() {
  // fill in extra columns (with real data?)
  mWobbles = 1.f;
  flags.wobbleType = WOBBLE_SHAKE;
}

void RoomView::StartSlide(Side side) {
  ASSERT(0 <= side && side < 4);
  mWobbles = 1.f;
  flags.wobbleType = WOBBLE_SLIDE_TOP + side;

}

//---------------------------------------------------------------
// SPRITE METHODS
//---------------------------------------------------------------

void RoomView::ShowPlayer() {
  if (gGame.GetPlayer().Equipment()) {
    mEquipSprite.setImage(Items, gGame.GetPlayer().Equipment()->itemId);
  }
  UpdatePlayer();
}

void RoomView::ShowItem(const ItemData* item) {
  auto& r = GetRoom();
  mTriggerSprite.setImage(Items, item->itemId);
  Int2 p = r.HasDepot() ? 
    16 * vec(r.Depot().tx+1, r.Depot().ty+1) : 
    16 * r.LocalCenter(0);
  mTriggerSprite.move(p.x-8, p.y);
}

void RoomView::ShowBlock(Sokoblock *pBlock) {
  mBlock = pBlock;
  mBlockSprite.setImage(pBlock->Asset());
  UpdateBlock();
}

void RoomView::SetPlayerImage(const PinnedAssetImage& img, unsigned frame) {
  mPlayerSprite.setImage(img, frame);
}

void RoomView::SetEquipPosition(Int2 p) {
  p += 16 * GetRoom().LocalCenter(0);
  mEquipSprite.setImage(Items, gGame.GetPlayer().Equipment()->itemId);
  mEquipSprite.move(p.x-8, p.y);
}
  
void RoomView::SetItemPosition(Int2 p) {
  p += 16 * GetRoom().LocalCenter(0);
  mTriggerSprite.move(p.x-8, p.y);
}

void RoomView::UpdatePlayer() {
  Int2 localPosition = gGame.GetPlayer().Position() - 128 * Location();
  mPlayerSprite.setImage(gGame.GetPlayer().Image(), gGame.GetPlayer().Frame());
  mPlayerSprite.move(localPosition.x-16, localPosition.y-16);
  if (gGame.GetPlayer().Equipment()) {
    mEquipSprite.move(localPosition.x-8, localPosition.y-ITEM_OFFSET);
  }
}

void RoomView::DrawPlayerFalling(int height) {
  Int2 localCenter = 16 * GetRoom().LocalCenter(0);
  mPlayerSprite.setImage(PlayerStand, 2);
  mPlayerSprite.move(localCenter.x-16, localCenter.y-32-height);
  if (gGame.GetPlayer().Equipment()) { 
    mEquipSprite.move(localCenter.x-8, localCenter.y-16-height-ITEM_OFFSET);
  }
}

void RoomView::UpdateBlock() {
  ASSERT(mBlock);
  const Int2 localPosition = mBlock->Position() - vec(32, 32) - 128 * Location();
  mBlockSprite.move(localPosition);
}

void RoomView::HidePlayer() {
  mPlayerSprite.hide();
  mEquipSprite.hide();
}

void RoomView::HideItem() { mTriggerSprite.hide(); }
void RoomView::HideEquip() { mEquipSprite.hide(); }
void RoomView::HideBlock() { 
  mBlockSprite.hide(); 
  mBlock = 0;
}

//----------------------------------------------------------------------
// TRAPDOOR METHODS
//----------------------------------------------------------------------

void RoomView::DrawTrapdoorFrame(int frame) {
  Int2 firstTile = GetRoom().LocalCenter(0) - vec(2,2);
  for(unsigned y=0; y<4; ++y)
  for(unsigned x=0; x<4; ++x) {
    mCanvas.bg0.image(
      vec(firstTile.x + x, firstTile.y + y) << 1,
      *(gGame.GetMap().Data().tileset),
      gGame.GetMap().GetTileId(mRoomId, vec(firstTile.x + x, firstTile.y + y))+frame
    );    
  }
}

//----------------------------------------------------------------------
// HELPERS
//----------------------------------------------------------------------

void RoomView::RefreshDoor() {
  auto& r = GetRoom();
  if (r.HasOpenDoor()) {
    for(int y=0; y<3; ++y)
    for(int x=3; x<5; ++x) {
      mCanvas.bg0.image(
        vec(x,y) << 1,
        *(gGame.GetMap().Data().tileset),
        gGame.GetMap().GetTileId(mRoomId, vec(x, y))+2
      );
    }
  }
}

void RoomView::RefreshDepot() {
  auto& r = GetRoom();
  if (r.HasDepotContents()) {
    const auto& depot = r.Depot();
    // assuming depots are door sizes (2x3)
    for(int y=depot.ty; y<depot.ty+3; ++y)
    for(int x=depot.tx; x<depot.tx+2; ++x) {
      mCanvas.bg0.image(
        vec(x,y) << 1,
        *(gGame.GetMap().Data().tileset),
        gGame.GetMap().GetTileId(mRoomId, vec(x, y))+2
      );
    }
    ShowItem(r.DepotContents());
  }

}

void RoomView::ShowFrame() {
  mCanvas.bg0.image(vec(0,0), FrameTop);
  mCanvas.bg0.image(vec(0, 1), FrameLeft);
  mCanvas.bg0.image(vec(15, 1), FrameRight);
  mCanvas.bg0.image(vec(0, 15), FrameBottom);
  HideOverlay();
}

void RoomView::DrawBackground() {
  mCanvas.bg0.setPanning(vec(0,0));
  Parent().DrawRoom(mRoomId);
  RefreshDoor();
  RefreshDepot();
  RefreshOverlay();
}

void RoomView::ComputeAnimatedTiles() {
  flags.animTileCount = 0;
  const unsigned tc = gGame.GetMap().Data().animatedTileCount;
  if (mRoomId == ROOM_UNDEFINED || tc == 0) { return; }
  const AnimatedTileData* pAnims = gGame.GetMap().Data().animatedTiles;
  for(unsigned lid=0; lid<64; ++lid) {
    uint8_t tid = gGame.GetMap().GetTileId(mRoomId, lid);
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
