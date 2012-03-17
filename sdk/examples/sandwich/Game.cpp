#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

bool Game::sNeighborDirty = false;
Game gGame;

#if PLAYTESTING_HACKS
float Game::sShakeTime = -1.f;
#endif

//------------------------------------------------------------------
// UPDATE CONTROLLERS AND PAINT (yielded from MainLoop)
//------------------------------------------------------------------

void Game::Paint(bool sync) {
  if (sNeighborDirty) { 
    CheckMapNeighbors(); 
  }
  float now = System::clock();
  float dt = now - mSimTime;
  mSimTime = now;
  mPlayer.Update(dt);
  for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->Update(dt);
  }
  if (sync || mNeedsSync) {
    System::paintSync();
    if (mNeedsSync > 0) {
      mNeedsSync--;
    }
  } else {
    System::paint();
  }
  mAnimFrames++;

  #if PLAYTESTING_HACKS
    Cube* pCube = mPlayer.View()->GetCube();
    _SYSShakeState shakeState;
    _SYS_getShake(pCube->id(), &shakeState);
    if (shakeState == SHAKING) {
      if (sShakeTime < 0.0f) {
        sShakeTime = 0.f;
      } else {
        sShakeTime += dt;
      }
    } else {
      sShakeTime = -1.f;
    }
  #endif
}

//------------------------------------------------------------------
// HELPERS
//------------------------------------------------------------------

void Game::MoveBlock(Sokoblock* block, Vec2 u) {
  if (block) {
    block->Move(u);
    mPlayer.TargetView()->UpdateBlock();

    // Could be optimized, perhaps
    const Cube::Side dir = InferDirection(u);
    ViewSlot* view = mPlayer.TargetView()->Parent()->VirtualNeighborAt(dir);
    if (view && view->IsShowingRoom()) {
      RoomView* pRoomView = view->GetRoomView();
      if (pRoomView->Block()) {
        pRoomView->UpdateBlock();
      } else {
        pRoomView->ShowBlock(block);
      }
    }
    //
  }
}


int Game::MovePlayerOneTile(Cube::Side dir, int progress, Sokoblock* block) {
  mPlayer.SetDirection(dir);

  const Vec2 unit = kSideToUnit[dir];
  if (dot(BroadDirection(), unit) != 1) { block = 0; }

  // if we have any progress to use, use it here
  if (progress != 0) {
    mPlayer.Move(progress * unit);
    MoveBlock(block, progress * unit);
    Paint();
  }
  // while we're still moving in this direction
  while(progress+WALK_SPEED < 16) {
    progress += WALK_SPEED;
    mPlayer.Move(WALK_SPEED * unit);
    MoveBlock(block, WALK_SPEED * unit);
    Paint();
  }
  // use up any remainder
  const int leftovers = 16 - progress;
  mPlayer.Move(leftovers * unit);
  MoveBlock(block, leftovers * unit);
  return WALK_SPEED - leftovers;  
}

void Game::MovePlayerAndRedraw(int dx, int dy) {
  mPlayer.SetDirection(InferDirection(Vec2(dx, dy)));
  mPlayer.Move(dx, dy);
  Paint();
}

void Game::WalkTo(Vec2 position, bool dosfx) {
  if (dosfx) {
    PlaySfx(sfx_running);
  }
  Vec2 delta = position - mPlayer.Position();
  while(delta.x > WALK_SPEED) {
    MovePlayerAndRedraw(WALK_SPEED, 0);
    delta.x -= WALK_SPEED;
  }
  while(delta.x < -WALK_SPEED) {
    MovePlayerAndRedraw(-WALK_SPEED, 0);
    delta.x += WALK_SPEED;
  }
  if (delta.x != 0) {
    MovePlayerAndRedraw(delta.x, 0);
  }
  while(delta.y > WALK_SPEED) {
    MovePlayerAndRedraw(0, WALK_SPEED);
    delta.y -= WALK_SPEED;
  }
  while(delta.y < WALK_SPEED) {
    MovePlayerAndRedraw(0, -WALK_SPEED);
    delta.y += WALK_SPEED;
  }
  if (delta.y != 0) {
    MovePlayerAndRedraw(0, delta.y);
  }
}

void Game::TeleportTo(const MapData& m, Vec2 position) {
  #if MUSIC_ON
    gChannelMusic.stop();
  #endif
  Vec2 room = position/128;
  ViewSlot* view = mPlayer.View();
  unsigned roomId = mPlayer.GetRoom()->Id();
  
  // blank other cubes
  for(ViewSlot* p = ViewBegin(); p != ViewEnd(); ++p) {
    if (p != view) { p->HideLocation(); }
  }

  IrisOut(view);
  mMap.SetData(m);
  mPlayer.SetPosition(position);
  if (pMinimap) { pMinimap->Restore(); }
  Zoom(view, room.x + room.y * mMap.Data()->width);
  
  ViewMode g = view->Graphics();
  g.init();
  view->GetCube()->vbuf.touch();

  // todo: expose music in level editor?
  PlayMusic(mMap.Data()->tileset == &TileSet_dungeon ? music_dungeon : music_castle);

  // walk out of the in-gate
  Vec2 target = mMap.GetRoom(room)->Center(0);
  mPlayer.SetDirection(InferDirection(target - position));
  view->ShowLocation(room, true);
  WalkTo(target, false);
  CheckMapNeighbors();

  // clear out any accumulated time
  mSimTime = System::clock();
}

void Game::IrisOut(ViewSlot* view) {
  view->HideSprites();
  BG1Helper(*view->GetCube()).Flush();
  ViewMode mode = view->Graphics();
  for(unsigned i=0; i<8; ++i) {
    for(unsigned x=i; x<16-i; ++x) {
      mode.BG0_putTile(Vec2(x, i), *BlackTile.tiles);
      mode.BG0_putTile(Vec2(x, 16-i-1), *BlackTile.tiles);
    }
    for(unsigned y=i+1; y<16-i-1; ++y) {
      mode.BG0_putTile(Vec2(i, y), *BlackTile.tiles);
      mode.BG0_putTile(Vec2(16-i-1, y), *BlackTile.tiles);
    }
    System::paintSync();
  }
  for(unsigned i=0; i<8; ++i) {
    view->GetCube()->vbuf.touch();
    System::paintSync();
  }
}

void Game::Zoom(ViewSlot* view, int roomId) {
  PlaySfx(sfx_zoomIn);
  VidMode_BG2 vid(view->GetCube()->vbuf);
  vid.BG2_setBorder(0x0000);
  vid.set();
  for(int x=0; x<8; ++x) {
    for(int y=0; y<8; ++y) {
      vid.BG2_drawAsset(
        Vec2(x<<1,y<<1),
        *(mMap.Data()->tileset),
        mMap.GetTileId(roomId, Vec2(x, y))
      );
    }
  }
  for (float t = 1.0f; t > 0.0f; t -= 0.05f) {
    AffineMatrix m = AffineMatrix::identity();
    m.translate(64, 64);
    m.scale(1.f+9.f*t);
    m.rotate(t * 1.1f);
    m.translate(-64, -64);
    vid.BG2_setMatrix(m);
    System::paint();
  }    
  System::paintSync();
}


void Game::NpcDialog(const DialogData& data, ViewSlot *vslot) {
    Dialog view;
    ViewMode mode = vslot->Graphics();
    PlaySfx(sfx_neighbor);
    for(unsigned i=0; i<8; ++i) { mode.hideSprite(i); }
    mode.BG0_drawAsset(Vec2(0,10), DialogBox);

    // save BG0 (above dialog line)
    VideoBuffer& vbuf = vslot->GetCube()->vbuf;
    uint16_t bg0_tiles[180];
    for(unsigned i=0; i<180; ++i) {
      bg0_tiles[i] = vbuf.peek( mode.BG0_addr(Vec2(Vec2(i%18, i/18))) );
    }

    for(unsigned line=0; line<data.lineCount; ++line) {
        const DialogTextData& txt = data.lines[line];
        if (line == 0 || data.lines[line-1].detail != txt.detail) {
          if (line > 0) {
            Paint(true);
            mode.setWindow(0, 80);
            _SYS_vbuf_write(&vbuf.sys, mode.BG0_addr(Vec2(0,0)), bg0_tiles, 180);
          }
          BG1Helper ovrly(*vslot->GetCube());
          ovrly.DrawAsset(Vec2(txt.detail == &NPC_Detail_pearl_detail ? 1 : 2, 0), *(txt.detail));
          ovrly.Flush();
          Paint(true);
          #if GFX_ARTIFACT_WORKAROUNDS
            vslot->GetCube()->vbuf.touch();
            Paint(true);
            vslot->GetCube()->vbuf.touch();
            Paint(true);
            vslot->GetCube()->vbuf.touch();
            Paint(true);
          #endif
          //Now set up a letterboxed 128x48 mode
          mode.setWindow(80, 48);
          view.Init(vslot->GetCube());
        }
        view.Erase();
        Paint(true);
        #if GFX_ARTIFACT_WORKAROUNDS
          vslot->GetCube()->vbuf.touch();
          Paint(true);
        #endif
        gGame.Paint(true);
        view.ShowAll(txt.line);
        if (line > 0) {
            PlaySfx(sfx_neighbor);
        }
        // fade in and out
        const unsigned hold = 250;
        for (unsigned i = 0; i < 16; i ++) {
            view.SetAlpha(i<<4);
            gGame.Paint();
        }
        view.SetAlpha(255);
        gGame.Paint();
        bool prev = vslot->GetCube()->touching();
        for (unsigned i = 0; i < hold; i++) {
            gGame.Paint();
            bool next = vslot->GetCube()->touching();
            if (next && !prev) { break; }
            prev = next;

        }
        for (unsigned i = 0; i < 16; i ++) {
            view.SetAlpha(0xff - (i<<4));
            gGame.Paint();
        }
        view.SetAlpha(0);
        gGame.Paint();
    }
    for(unsigned i=0; i<16; ++i) {
        gGame.Paint();
    }
    PlaySfx(sfx_deNeighbor);
}

void Game::DescriptionDialog(const char* hdr, const char* msg, ViewSlot* pView) {
  ViewMode gfx = pView->Graphics();
  #if GFX_ARTIFACT_WORKAROUNDS    
    Paint(true);
    pView->GetCube()->vbuf.touch();
    Paint(true);
  #endif
  gfx.setWindow(80+16,128-80-16);
  Dialog view;
  view.Init(pView->GetCube());
  view.Erase();
  if (hdr) { view.Show(hdr); }
  view.ShowAll(msg);
  pView->GetCube()->vbuf.touch();
  Paint(true);
  for(int t=0; t<16; t++) {
    gfx.setWindow(80+15-(t),128-80-15+(t));
    view.SetAlpha(t<<4);
    Paint();
  }
  view.SetAlpha(255);
  for(float t=System::clock(); System::clock()-t<4.f && !pView->Touched();) { Paint(); }
  pView->GetCube()->vbuf.touch();
  Paint(true);
  mPlayer.CurrentView()->Parent()->Restore();
  mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.index+ (SIDE_BOTTOM<<4));
  #if GFX_ARTIFACT_WORKAROUNDS    
    Paint(true);
    pView->GetCube()->vbuf.touch();
    Paint(true);
  #endif
  // wait a sec
  for(float t=System::clock(); System::clock()-t<0.25f;) { Paint(); }
}

//------------------------------------------------------------------
// EVENTS
//------------------------------------------------------------------

void Game::OnInventoryChanged() {
  for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->RefreshInventory();
  }

  
  // demo end-condition hack
  int count = 0;
  for(int i=0; i<4; ++i) {
    if(!mState.HasItem(i)) {
      return;
    }
  }
  
  mIsDone = true;
}


void Game::OnPickup(Room *pRoom) {
  const ItemData* pItem = pRoom->TriggerAsItem();
  const ItemTypeData &itemType = gItemTypeData[pItem->itemId];
  if (itemType.storageType == STORAGE_EQUIPMENT) {

    //---------------------------------------------------------------------------
    // PLAYER TRIGGERED EQUIP PICKUP
    pRoom->ClearTrigger();
    mPlayer.CurrentView()->HideItem();
    if (mPlayer.Equipment()) {
      OnDropEquipment(pRoom);
    }
    PlaySfx(sfx_pickup);
    mPlayer.SetEquipment(pItem);
    // do a pickup animation
    for(unsigned frame=0; frame<PlayerPickup.frames; ++frame) {
      mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index + (frame<<4));
      float t=System::clock();
      Paint();
      do {
        // this calc is kinda annoyingly complex
        float u = (mSimTime - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.frames;
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetEquipPosition(Vec2(0, -float(ITEM_OFFSET) * u) );
      } while(mSimTime-t<0.075f);
    }
    mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.index+ (SIDE_BOTTOM<<4));
    DescriptionDialog(
      "ITEM DISCOVERED", 
      itemType.description, 
      mPlayer.CurrentView()->Parent()
    );
  } else {
    //-----------------------------------------------------------------------
    // PLAYER TRIGGERED ITEM OR KEY PICKUP
    if (mState.FlagTrigger(pItem->trigger)) { pRoom->ClearTrigger(); }
    if (mState.PickupItem(pItem->itemId)) {
      PlaySfx(sfx_pickup);
      OnInventoryChanged();
    }
    // do a pickup animation
    for(unsigned frame=0; frame<PlayerPickup.frames; ++frame) {
      mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index + (frame<<4));
      float t=System::clock();
      Paint();
      do {
        // this calc is kinda annoyingly complex
        float u = (mSimTime - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.frames;
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetItemPosition(Vec2(0, -36.f * u) );
      } while(System::clock()-t<0.075f);
    }
    mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.index+ (SIDE_BOTTOM<<4));
    DescriptionDialog(
      "ITEM DISCOVERED", 
      itemType.description, 
      mPlayer.CurrentView()->Parent()
    );
    mPlayer.CurrentView()->HideItem();        
  }

  OnTriggerEvent(pItem->trigger.eventType);
}

unsigned Game::OnPassiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasItem()) {
    OnPickup(pRoom);
  } else if (pRoom->HasTrapdoor()) {

    //-------------------------------------------------------------------------
    // PLAYER TRIGGERED TRAPDOOR
    // animate the tiles opening
    Vec2 firstTile = pRoom->LocalCenter(0) - Vec2(2,2);
    for(unsigned i=1; i<=7; ++i) { // magic
      mPlayer.CurrentView()->DrawTrapdoorFrame(i);
      Paint(true);
      Paint(true);
    }
    // animate pearl falling TODO
    mPlayer.CurrentView()->HidePlayer();
    for(unsigned i=0; i<16; ++i) {
      Paint(true);
    }
    // animate the tiles closing
    for(int i=6; i>=0; --i) { // magic
      mPlayer.CurrentView()->DrawTrapdoorFrame(i);
      Paint(true);
      Paint(true);
    }
    // pan to respawn point
    ViewSlot *pView = mPlayer.CurrentView()->Parent();
    for(ViewSlot* p=ViewBegin(); p!=ViewEnd(); ++p) {
      if (p != pView) { p->HideLocation(); }
    }
    pView->HideSprites();
    BG1Helper(*pView->GetCube()).Flush();
    Paint(true);

    Room* targetRoom = mMap.GetRoom(pRoom->Trapdoor()->respawnRoomId);
    Vec2 start = 128 * pRoom->Location();
    Vec2 delta = 128 * (targetRoom->Location() - pRoom->Location());
    ViewMode mode = pView->Graphics();
    float t=mSimTime; 
    do {
      float u = (mSimTime-t) / 2.333f;
      u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
      Vec2 pos = Vec2(start.x + int(u * delta.x), start.y + int(u * delta.y));
      DrawOffsetMap(&mode, mMap.Data(), pos);
      Paint(true);
    } while(mSimTime-t<2.333f);
    // fall
    DrawRoom(&mode, mMap.Data(), targetRoom->Id());
    int animHeights[] = { 48, 32, 16, 0, 8, 12, 16, 12, 8, 0 };
    for(unsigned i=0; i<arraysize(animHeights); ++i) {
      mPlayer.CurrentView()->DrawPlayerFalling(animHeights[i]);
      Paint(true);
    }
    mPlayer.SetPosition(targetRoom->Center(0));
    mPlayer.SetDirection(SIDE_BOTTOM);
    pView->ShowLocation(mPlayer.Position()/128, true);
    CheckMapNeighbors();
    Paint(true);
    return TRIGGER_RESULT_PATH_INTERRUPTED;
  }
  return TRIGGER_RESULT_NONE;
}

void Game::OnEnterGateway(Room*pRoom) {
  //---------------------------------------------------------------------------
  // PLAYER TRIGGERED GATEWAY
  const GatewayData* pGate = pRoom->TriggerAsGate();
  const MapData& targetMap = gMapData[pGate->targetMap];
  const GatewayData& pTargetGate = targetMap.gates[pGate->targetGate];
  if (mState.FlagTrigger(pGate->trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
  WalkTo(128 * mPlayer.GetRoom()->Location() + Vec2(pGate->x, pGate->y));
  mPlayer.SetEquipment(0);
  TeleportTo(gMapData[pGate->targetMap], Vec2(
    128 * (pTargetGate.trigger.room % targetMap.width) + pTargetGate.x,
    128 * (pTargetGate.trigger.room / targetMap.width) + pTargetGate.y
  ));
  OnTriggerEvent(pGate->trigger.eventType);
  RestorePearlIdle();
}

void Game::OnNpcChatter(Room *pRoom) {
  //-------------------------------------------------------------------------
  // PLAYER TRIGGERED NPC DIALOG
  mPlayer.SetStatus(PLAYER_STATUS_IDLE);
  mPlayer.CurrentView()->UpdatePlayer();
  for(int i=0; i<16; ++i) { Paint(true); }
  const NpcData* pNpc = pRoom->TriggerAsNPC();
  if (mState.FlagTrigger(pNpc->trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
  NpcDialog(gDialogData[pNpc->dialog], mPlayer.CurrentView()->Parent());
  System::paintSync();
  mPlayer.CurrentView()->Parent()->Restore();
  System::paintSync();
  OnTriggerEvent(pNpc->trigger.eventType);
  RestorePearlIdle();
}
void Game::OnActiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasGateway()) {
    OnEnterGateway(pRoom);
  } else if (pRoom->HasNPC()) {
    OnNpcChatter(pRoom);
  }  
}

void Game::RestorePearlIdle() {
  if (mPlayer.Direction() != SIDE_BOTTOM || mPlayer.Status() != PLAYER_STATUS_IDLE) {
    // Always look "south" after an action
    mPlayer.SetDirection(SIDE_BOTTOM);
    mPlayer.SetStatus(PLAYER_STATUS_IDLE);
    mPlayer.CurrentView()->UpdatePlayer();
  }
}

void Game::OnDropEquipment(Room *pRoom) {
  const ItemData *pItem = mPlayer.Equipment();
  // TODO: Putting-Down Animation (pickup backwards?)
  mPlayer.SetEquipment(0);
  pRoom->SetTrigger(TRIGGER_ITEM, &pItem->trigger);
  mPlayer.CurrentView()->HideEquip();
  mPlayer.CurrentView()->ShowItem();
}

void Game::OnUseEquipment() {
  // TODO: special cases for different types of equipment
  Room *pRoom = mPlayer.GetRoom();
  if (pRoom->HasItem()) {
    OnPickup(pRoom);
  } else {
    OnDropEquipment(pRoom); // default
  }
  
}

void Game::OnTriggerEvent(unsigned id) {
  switch(id) {
    case EVENT_ADVANCE_QUEST_AND_REFRESH:
      mState.AdvanceQuest();
      mMap.RefreshTriggers();
      for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
        if (p->IsShowingRoom()) {
          p->Restore();
        }
      }
      break;
    case EVENT_ADVANCE_QUEST_AND_TELEPORT:
      mState.AdvanceQuest();
      const QuestData* quest = mState.Quest();
      const MapData& map = gMapData[quest->mapId];
      const RoomData& room = map.rooms[quest->roomId];
      TeleportTo(map, Vec2 (
        128 * (quest->roomId % map.width) + 16 * room.centerX,
        128 * (quest->roomId / map.width) + 16 * room.centerY
      ));
      break;
  }
}

bool Game::OnEncounterBlock(Sokoblock* block) {
  //-------------------------------------------------------------
  // BLOCK PUSHING
  // determine if the block can be pushed:
  //  i.    ? is there a portal to the blockTarget room? ?
  //  ii.   is the blockTarget not subdivided?
  //  iii.  ? portal "big enough" check ?
  //  iv.   targetRoom does not already have a block
  const Vec2 blockTargetLoc = mPlayer.TargetRoom()->Location() + BroadDirection();
  if (!mMap.Contains(blockTargetLoc)) { return false; }
  Room* pRoom = mMap.GetRoom(blockTargetLoc);
  if (pRoom->IsSubdivided()) { return false; }
  // TODO CHECK ROOM DOES NOT ALREADY CONTAIN A BLOCK
  //    -- MOVE ROOMVIEW::CHECKBLOCK --> ROOM
  //for (Sokoblock* p=mMap.BlockBegin(); p!=mMap.BlockEnd(); ++p) {
  //}
  return true;
}

//------------------------------------------------------------------
// NEIGHBOR WALKING
//------------------------------------------------------------------

#define VIEW_UNVISITED 0
#define VIEW_UNCHANGED 1
#define VIEW_CHANGED 2

static bool VisitMapView(uint8_t* visited, ViewSlot* view, Vec2 loc, ViewSlot* origin=0) {
  if (!view || visited[view->GetCubeID()]) { return false; }
  if (origin) { view->GetCube()->orientTo(*(origin->GetCube())); }
  bool result = view->ShowLocation(loc, false, false);
  visited[view->GetCubeID()] = result ? VIEW_CHANGED:VIEW_UNCHANGED;
  for(Cube::Side i=0; i<NUM_SIDES; ++i) {
    result |= VisitMapView(visited, view->VirtualNeighborAt(i), loc+kSideToUnit[i], view);
  }
  return result;
}

void Game::CheckMapNeighbors() {
  ViewSlot *root = mPlayer.View();
  if (!root->IsShowingRoom()) { return; }
  uint8_t visited[NUM_CUBES];
  for(unsigned i=0; i<NUM_CUBES; ++i) { visited[i] = 0; }
  bool chchchchanges = VisitMapView(visited, root, root->GetRoomView()->Location());
  
  if (chchchchanges) {
    PlaySfx(sfx_neighbor);
  }

  bool otherChanges = false;
  for(ViewSlot* v = ViewBegin(); v!=ViewEnd(); ++v) {
    if (!visited[v->GetCubeID()] && v->HideLocation(false)) { 
      otherChanges = true;
      visited[v->GetCubeID()] = VIEW_CHANGED;
    }
  }

  if (otherChanges && !chchchchanges) {
    PlaySfx(sfx_deNeighbor);    
  }
  
  sNeighborDirty = false;

  if (chchchchanges || otherChanges) {
    #if GFX_ARTIFACT_WORKAROUNDS
      Paint(true);
      for(ViewSlot *v=ViewBegin(); v!=ViewEnd(); ++v) {
        //if (visited[v->GetCubeID()] == VIEW_CHANGED) {
          v->GetCube()->vbuf.touch();
        //}
      }
      Paint(true);
      for(ViewSlot *v=ViewBegin(); v!=ViewEnd(); ++v) {
        //if (visited[v->GetCubeID()] == VIEW_CHANGED) {
          v->GetCube()->vbuf.touch();
        //}
      }
    #endif
    Paint(true);
  }


}
