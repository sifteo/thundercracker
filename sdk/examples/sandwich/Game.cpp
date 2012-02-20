#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

#define RESULT_NONE             0
#define RESULT_PATH_INTERRUPTED 1

static bool sNeighborDirty = false;

static void onNeighbor(void *context,
    Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {
  sNeighborDirty = true;
}

static void onTouch(_SYSCubeID cid) {    
    // /pGame->ViewAt(cid)->touched = !pGame->ViewAt(cid)->touched;
}

void Game::ObserveNeighbors(bool flag) {
  if (flag) {
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighbor, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighbor, NULL);
  } else {
    _SYS_setVector(_SYS_NEIGHBOR_ADD, NULL, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, NULL, NULL);
  }
}

//------------------------------------------------------------------
// MAIN PLAYER INTERACTION LOOP
//------------------------------------------------------------------

void Game::MainLoop(Cube* pPrimary) {
  // reset everything
  mSimFrames = 0;
  mAnimFrames = 0;
  mIsDone = false;
  mNeedsSync = 0;
  mState.Init();
  mMap.Init();
  mPlayer.Init(pPrimary);
  for(ViewSlot* v = ViewBegin(); v!=ViewEnd(); ++v) { 
    if (v->GetCube() != pPrimary) {
      v->Init(); 
    }
  }
  Zoom(mPlayer.View(), mPlayer.GetRoom()->Id());
  mPlayer.View()->ShowLocation(mPlayer.Location());
  PlayMusic(music_castle);
  mSimTime = System::clock();
  ObserveNeighbors(true);
  CheckMapNeighbors();
  while(!mIsDone) {
    // wait for touch
    mPlayer.SetStatus(PLAYER_STATUS_IDLE);
    mPlayer.CurrentView()->UpdatePlayer();
    mPath.Cancel();
    do {
      Update();
      if (mPlayer.CurrentView()->Parent()->Touched()) {
        OnActiveTrigger();
      }
    } while (!pGame->GetMap()->FindBroadPath(&mPath));
    // go to the target
    mPlayer.SetStatus(PLAYER_STATUS_WALKING);
    do {
      if (mPath.IsDefined()) {
        mPlayer.SetDirection(mPath.steps[0]);
        mMap.GetBroadLocationNeighbor(*mPlayer.Current(), mPlayer.Direction(), mPlayer.Target());
      }
      // animate walking to target
      PlaySfx(sfx_running);
      mPlayer.TargetView()->ShowPlayer();
      if (mPlayer.Direction() == SIDE_TOP && mPlayer.GetRoom()->HasClosedDoor()) {
        int progress;
        for(progress=0; progress<24; progress+=WALK_SPEED) {
          mPlayer.Move(0, -WALK_SPEED);
          Update();
        }
        if (mState.HasBasicKey()) {
          mState.DecrementBasicKeyCount();
          if (!mState.HasBasicKey()) { OnInventoryChanged(); }
          // check the door
          mPlayer.GetRoom()->OpenDoor();
          mPlayer.CurrentView()->DrawBackground();
          mPlayer.CurrentView()->Parent()->GetCube()->vbuf.touch();
          mPlayer.CurrentView()->UpdatePlayer();
          float timeout = System::clock();
          NeedsSync();
          do {
            Update();
          } while(System::clock() - timeout <  0.5f);
          PlaySfx(sfx_doorOpen);
          // finish up
          for(; progress+WALK_SPEED<=128; progress+=WALK_SPEED) {
            mPlayer.Move(0,-WALK_SPEED);
            Update();
          }
          // fill in the remainder
          mPlayer.SetPosition(
            mPlayer.GetRoom()->Center(mPlayer.Target()->subdivision)
          );
        } else {
          PlaySfx(sfx_doorBlock);
          mPath.Cancel();
          mPlayer.ClearTarget();
          mPlayer.SetDirection( (mPlayer.Direction()+2)%4 );
          for(progress=0; progress<24; progress+=WALK_SPEED) {
            Update();
            mPlayer.Move(0, WALK_SPEED);
          }          
        }
      } else { // general case - A*
        if (mPlayer.TargetView()->GetRoom()->IsBridge()) {
          mPlayer.TargetView()->HideOverlay(mPlayer.Direction()%2 == 1);
        }
        bool result = pGame->GetMap()->FindNarrowPath(*mPlayer.Current(), mPlayer.Direction(), &mMoves);
        ASSERT(result);
        int progress = 0;
        uint8_t *pNextMove;
        for(pNextMove=mMoves.pFirstMove; pNextMove!=mMoves.End(); ++pNextMove) {
          mPlayer.SetDirection(*pNextMove);
          if (progress != 0) {
            mPlayer.Move(progress * kSideToUnit[*pNextMove]);
            Update();
          }
          while(progress+WALK_SPEED < 16) {
            progress += WALK_SPEED;
            mPlayer.Move(WALK_SPEED * kSideToUnit[*pNextMove]);
            Update();
          }
          mPlayer.Move((16 - progress) * kSideToUnit[*pNextMove]);
          progress = WALK_SPEED - (16-progress);
        }
        if (progress != 0) {
          pNextMove--;
          mPlayer.Move(progress * kSideToUnit[*pNextMove]);
          progress = 0;
          Update();
        }
      }
      if (mPlayer.TargetView()) { // did we land on the target?
        mPlayer.AdvanceToTarget();
        if (OnPassiveTrigger() == RESULT_PATH_INTERRUPTED) {
          mPath.Cancel();
        }
      }  
    } while(mPath.PopStep(*mPlayer.Current(), mPlayer.Target()));
    OnActiveTrigger();
  }
}

//------------------------------------------------------------------
// UPDATE CONTROLLERS AND PAINT (yielded from MainLoop)
//------------------------------------------------------------------

void Game::Update(bool sync) {
  mDeltaTime = UpdateDeltaTime();
  // update any sub-controllers here (e.g. inv menu)
  Paint(sync);
  mSimFrames++;
}

//------------------------------------------------------------------
// JUST UPDATE VIEWS
//------------------------------------------------------------------

void Game::Paint(bool sync) {
    if (sNeighborDirty) { 
      CheckMapNeighbors(); 
    }
    mPlayer.Update(mDeltaTime);
    for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->Update();
    #if KLUDGES
    p->GetCube()->vbuf.touch();
    #endif
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
}

//------------------------------------------------------------------
// HELPERS
//------------------------------------------------------------------

float Game::UpdateDeltaTime() {
  float now = System::clock();
  float result = now - mSimTime;
  mSimTime = now;
  return result;
}

void Game::MovePlayerAndRedraw(int dx, int dy) {
  mPlayer.SetDirection(InferDirection(Vec2(dx, dy)));
  mPlayer.Move(dx, dy);
  mPlayer.Update(UpdateDeltaTime());
  System::paint();
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
  gChannelMusic.stop();
  Vec2 room = position/128;
  ViewSlot* view = mPlayer.View();
  unsigned roomId = mPlayer.GetRoom()->Id();
  
  // blank other cubes
  for(ViewSlot* p = ViewBegin(); p != ViewEnd(); ++p) {
    if (p != view) { p->HideLocation(); }
  }

  IrisOut(view);
  mMap.SetData(m);
  Zoom(view, room.x + room.y * mMap.Data()->width);
  
  // todo: expose music in level editor?
  PlayMusic(mMap.Data() == &gMapData[1] ? music_dungeon : music_castle);

  // walk out of the in-gate
  Vec2 target = mMap.GetRoom(room)->Center(0);
  mPlayer.SetPosition(position);
  mPlayer.SetDirection(InferDirection(target - position));
  view->ShowLocation(room);
  WalkTo(target, false);
  CheckMapNeighbors();

  // clear out any accumulated time
  UpdateDeltaTime();
}

void Game::IrisOut(ViewSlot* view) {
  view->HideSprites();
  view->Overlay().Flush();
  ViewMode mode = view->Graphics();
  for(unsigned i=0; i<8; ++i) {
    for(unsigned x=i; x<16-i; ++x) {
      mode.BG0_putTile(Vec2(x, i), *Black.tiles);
      mode.BG0_putTile(Vec2(x, 16-i-1), *Black.tiles);
    }
    for(unsigned y=i+1; y<16-i-1; ++y) {
      mode.BG0_putTile(Vec2(i, y), *Black.tiles);
      mode.BG0_putTile(Vec2(16-i-1, y), *Black.tiles);
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

//------------------------------------------------------------------
// EVENTS
//------------------------------------------------------------------

void Game::OnInventoryChanged() {
  for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->RefreshInventory();
  }
  const int firstSandwichId = 2;
  int count = 0;
  for(int i=firstSandwichId; i<firstSandwichId+4; ++i) {
    if(!mState.HasItem(i)) {
      return;
    }
  }
  mIsDone = true;
}


unsigned Game::OnPassiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasItem()) {
    const ItemData* pItem = pRoom->TriggerAsItem();
    if (mState.FlagTrigger(pItem->trigger)) { pRoom->ClearTrigger(); }
    if (mState.PickupItem(pItem->itemId)) {
      PlaySfx(sfx_pickup);
      OnInventoryChanged();
    }
    // do a pickup animation
    for(unsigned frame=0; frame<PlayerPickup.frames; ++frame) {
      mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index + (frame * PlayerPickup.width * PlayerPickup.height));
      float t=System::clock();
      do {
        // this calc is kinda annoyingly complex
        float u = (System::clock() - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.frames;
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetItemPosition(Vec2(0, -36.f * u) );
      } while(System::clock()-t<0.075);
    }
    mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.index+ SIDE_BOTTOM* PlayerStand.width * PlayerStand.height);
    for(float t=System::clock(); System::clock()-t<0.25f;) { System::paint(); }
    mPlayer.CurrentView()->HideItem();        
  } else if (pRoom->HasTrapdoor()) {
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
    pView->Overlay().Flush();
    Paint(true);

    Room* targetRoom = mMap.GetRoom(pRoom->Trapdoor()->respawnRoomId);
    Vec2 start = 128 * pRoom->Location();
    Vec2 delta = 128 * (targetRoom->Location() - pRoom->Location());
    ViewMode mode = pView->Graphics();
    float t=System::clock(); 
    do {
      float u = (System::clock()-t) / 2.333f;
      u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
      Vec2 pos = Vec2(start.x + int(u * delta.x), start.y + int(u * delta.y));
      DrawOffsetMap(&mode, mMap.Data(), pos);
      Paint(true);
    } while(System::clock()-t<2.333f);
    // fall
    DrawRoom(&mode, mMap.Data(), targetRoom->Id());
    int animHeights[] = { 48, 32, 16, 0, 8, 12, 16, 12, 8, 0 };
    for(unsigned i=0; i<arraysize(animHeights); ++i) {
      mPlayer.CurrentView()->DrawPlayerFalling(animHeights[i]);
      Paint(true);
    }
    mPlayer.SetPosition(targetRoom->Center(0));
    mPlayer.SetDirection(SIDE_BOTTOM);
    pView->ShowLocation(mPlayer.Location());
    CheckMapNeighbors();
    Paint(true);
    return RESULT_PATH_INTERRUPTED;
  }

  return RESULT_NONE;
}

void Game::OnActiveTrigger() {
  if (mPlayer.GetRoom()->HasGateway()) {
    const GatewayData* pGate = mPlayer.GetRoom()->TriggerAsGate();
    const MapData& targetMap = gMapData[pGate->targetMap];
    const GatewayData& pTargetGate = targetMap.gates[pGate->targetGate];
    if (mState.FlagTrigger(pGate->trigger)) { mPlayer.GetRoom()->ClearTrigger(); } 
    WalkTo(128 * mPlayer.GetRoom()->Location() + Vec2(pGate->x, pGate->y));
    TeleportTo(gMapData[pGate->targetMap], Vec2(
      128 * (pTargetGate.trigger.room % targetMap.width) + pTargetGate.x,
      128 * (pTargetGate.trigger.room / targetMap.width) + pTargetGate.y
    ));
  } else if (mPlayer.GetRoom()->HasNPC()) {
    ////////
    mPlayer.SetStatus(PLAYER_STATUS_IDLE);
    mPlayer.CurrentView()->UpdatePlayer();
    for(int i=0; i<16; ++i) { Paint(true); }
    const NpcData* pNpc = mPlayer.GetRoom()->TriggerAsNPC();
    if (mState.FlagTrigger(pNpc->trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
    DoDialog(gDialogData[pNpc->dialog], mPlayer.CurrentView()->Parent()->GetCube());
    System::paintSync();
    mPlayer.CurrentView()->Parent()->Restore();
    System::paintSync();
  }  
  if (mPlayer.Direction() != SIDE_BOTTOM || mPlayer.Status() != PLAYER_STATUS_IDLE) {
    mPlayer.SetDirection(SIDE_BOTTOM);
    mPlayer.SetStatus(PLAYER_STATUS_IDLE);
    mPlayer.CurrentView()->UpdatePlayer();
  }
}

//------------------------------------------------------------------
// NEIGHBOR WALKING
//------------------------------------------------------------------

static void VisitMapView(uint8_t* visited, ViewSlot* view, Vec2 loc, ViewSlot* origin=0) {
  if (!view || visited[view->GetCubeID()]) { return; }
  visited[view->GetCubeID()] = true;
  if (view->ShowLocation(loc)) {
    PlaySfx(sfx_neighbor);
  }
  if (origin) {
    view->GetCube()->orientTo(*(origin->GetCube()));
  }
  for(Cube::Side i=0; i<NUM_SIDES; ++i) {
    VisitMapView(visited, view->VirtualNeighborAt(i), loc+kSideToUnit[i], view);
  }
}

void Game::CheckMapNeighbors() {
  uint8_t visited[NUM_CUBES];
  for(unsigned i=0; i<NUM_CUBES; ++i) { visited[i] = 0; }
  VisitMapView(visited, mPlayer.View(), mPlayer.Location());
  for(ViewSlot* v = ViewBegin(); v!=ViewEnd(); ++v) {
    if (!visited[v->GetCubeID()]) { 
      if (v->HideLocation()) {
        PlaySfx(sfx_deNeighbor);
      }
    }
  }
  sNeighborDirty = false;
}
