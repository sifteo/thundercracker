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
  Parent()->HideSprites();
  ViewMode mode = Parent()->Graphics();
  Map& map = *gGame.GetMap();
  flags.hideOverlay = false;
  mRoomId = roomId;
  // are we showing an items?
  mStartFrame = gGame.AnimFrame();
  ComputeAnimatedTiles();
  Room* r = GetRoom();
  switch(r->TriggerType()) {
    case TRIGGER_ITEM: 
      ShowItem();
      break;
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
      mode.moveSprite(BFF_SPRITE_ID, mAmbient.bff.x-68, mAmbient.bff.y-68);
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

void RoomView::Restore() {
  Init(mRoomId);
}

void RoomView::Update(float dt) {
  ViewMode mode = Parent()->Graphics();
  // update animated tiles (could suffer some optimization)
  const unsigned t = gGame.AnimFrame() - mStartFrame;
  for(unsigned i=0; i<mAnimTileCount; ++i) {
    const AnimTile& view = mAnimTiles[i];
    const unsigned localt = t % (view.frameCount << 2);
    if (localt % 4 == 0) {
        mode.BG0_drawAsset(
          Vec2((view.lid%8)<<1,(view.lid>>3)<<1),
          *(gGame.GetMap()->Data()->tileset),
          gGame.GetMap()->Data()->rooms[mRoomId].tiles[view.lid] + (localt>>2)
        );
    }
  }

  if (gGame.GetMap()->Data()->ambientType && mAmbient.bff.active) {
    mAmbient.bff.Update();
    mode.moveSprite(BFF_SPRITE_ID, mAmbient.bff.x-68, mAmbient.bff.y-68);
    mode.setSpriteImage(
      BFF_SPRITE_ID, 
      Butterfly.index + 4 * mAmbient.bff.dir + mAmbient.bff.frame / 3
    );
  }

  // item hover
  if (GetRoom()->HasItem()) {
    const unsigned hoverTime = (gGame.AnimFrame() - mStartFrame) % HOVER_COUNT;
    Vec2 p = 16 * GetRoom()->LocalCenter(0);
    mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y + kHoverTable[hoverTime]);
  }
}

//----------------------------------------------------------------
// ROOM METHODS
//----------------------------------------------------------------

Room* RoomView::GetRoom() const { 
  return gGame.GetMap()->GetRoom(mRoomId);
}

Vec2 RoomView::Location() const {
  return gGame.GetMap()->GetLocation(mRoomId);
}

bool RoomView::GatewayTouched() const {
  const Room* pRoom = GetRoom();
  if (pRoom->HasGateway()) {
    const Cube::Side side = ComputeGateSide(pRoom->TriggerAsGate());
    if (side != SIDE_UNDEFINED) {
      const ViewSlot *view = Parent()->VirtualNeighborAt(side);
      return view && view->Touched() && view->IsShowingGatewayEdge();
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

void RoomView::ShowItem() {
  Room* pRoom = GetRoom();
  ASSERT(pRoom->HasItem());
  ViewMode mode = Parent()->Graphics();
  mode.setSpriteImage(TRIGGER_SPRITE_ID, Items, pRoom->TriggerAsItem()->itemId);
  Vec2 p = 16 * pRoom->LocalCenter(0);
  mode.moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void RoomView::ShowBlock(Sokoblock *pBlock) {
  mBlock = pBlock;
  Parent()->Graphics().setSpriteImage(BLOCK_SPRITE_ID, CompanionCube);
  UpdateBlock();
}

void RoomView::SetPlayerFrame(unsigned frame) {
  Parent()->Graphics().setSpriteImage(PLAYER_SPRITE_ID, frame);
}

void RoomView::SetEquipPosition(Vec2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  ViewMode gfx = Parent()->Graphics();
  gfx.setSpriteImage(EQUIP_SPRITE_ID, Items, gGame.GetPlayer()->Equipment()->itemId);
  gfx.moveSprite(EQUIP_SPRITE_ID, p.x-8, p.y);
}
  
void RoomView::SetItemPosition(Vec2 p) {
  p += 16 * GetRoom()->LocalCenter(0);
  Parent()->Graphics().moveSprite(TRIGGER_SPRITE_ID, p.x-8, p.y);
}

void RoomView::UpdatePlayer() {
  Vec2 localPosition = gGame.GetPlayer()->Position() - 128 * Location();
  ViewMode gfx = Parent()->Graphics();
  gfx.setSpriteImage(PLAYER_SPRITE_ID, gGame.GetPlayer()->AnimFrame());
  gfx.moveSprite(PLAYER_SPRITE_ID, localPosition.x-16, localPosition.y-16);
  if (gGame.GetPlayer()->Equipment()) {
    gfx.moveSprite(EQUIP_SPRITE_ID, localPosition.x-8, localPosition.y-ITEM_OFFSET);
  }
}

void RoomView::DrawPlayerFalling(int height) {
  ViewMode mode = Parent()->Graphics();
  Vec2 localCenter = 16 * GetRoom()->LocalCenter(0);
  mode.setSpriteImage(PLAYER_SPRITE_ID, PlayerStand.index + (2<<4));
  mode.moveSprite(PLAYER_SPRITE_ID, localCenter.x-16, localCenter.y-32-height);
  mode.resizeSprite(PLAYER_SPRITE_ID, 32, 32);
  if (gGame.GetPlayer()->Equipment()) { 
    mode.moveSprite(EQUIP_SPRITE_ID, localCenter.x-8, localCenter.y-16-height-ITEM_OFFSET);
  }
}

void RoomView::UpdateBlock() {
  ASSERT(mBlock);
  const Vec2 localPosition = mBlock->Position() - Vec2(32, 32) - 128 * Location();
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
  Vec2 firstTile = GetRoom()->LocalCenter(0) - Vec2(2,2);
  for(unsigned y=0; y<4; ++y)
  for(unsigned x=0; x<4; ++x) {
    mode.BG0_drawAsset(
      Vec2(firstTile.x + x, firstTile.y + y) << 1,
      *(gGame.GetMap()->Data()->tileset),
      gGame.GetMap()->GetTileId(mRoomId, Vec2(firstTile.x + x, firstTile.y + y))+frame
    );    
  }
}

//----------------------------------------------------------------------
// HELPERS
//----------------------------------------------------------------------

void RoomView::DrawBackground() {
  ViewMode mode = Parent()->Graphics();
  mode.BG0_setPanning(Vec2(0,0));
  DrawRoom(&mode, gGame.GetMap()->Data(), mRoomId)  ;

  // hack alert!
  const Room *pRoom = GetRoom();
  if (pRoom->HasOpenDoor()) {
    for(int y=0; y<3; ++y) {
      for(int x=3; x<=4; ++x) {
        mode.BG0_drawAsset(
          Vec2(x,y) << 1,
          *(gGame.GetMap()->Data()->tileset),
          gGame.GetMap()->GetTileId(mRoomId, Vec2(x, y))+2
        );
      }
    }
  }
  BG1Helper ovrly(*(Parent()->GetCube()));
  if (!flags.hideOverlay && pRoom->HasOverlay()) {
    DrawRoomOverlay(&ovrly, gGame.GetMap()->Data(), pRoom->OverlayTile(), pRoom->OverlayBegin());
  }
  if (pRoom->TriggerType() == TRIGGER_NPC) {
      const NpcData* npc = pRoom->TriggerAsNPC();
      const DialogData& dialog = gDialogData[npc->dialog];
      ovrly.DrawAsset(Vec2((npc->x-16)>>3, (npc->y-16)>>3), *dialog.npc);
  }

  ovrly.Flush();
}

void RoomView::ComputeAnimatedTiles() {
  mAnimTileCount = 0;
  const unsigned tc = gGame.GetMap()->Data()->animatedTileCount;
  if (mRoomId == ROOM_UNDEFINED || tc == 0) { return; }
  const AnimatedTileData* pAnims = gGame.GetMap()->Data()->animatedTiles;
  for(unsigned lid=0; lid<64; ++lid) {
    uint8_t tid = gGame.GetMap()->Data()->rooms[mRoomId].tiles[lid];
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
