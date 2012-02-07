#include "Game.h"

static bool sNeighborDirty = false;

static void onNeighbor(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {
  sNeighborDirty = true;
}

static void onTouch(_SYSCubeID cid) {    
    // /pGame->ViewAt(cid)->touched = !pGame->ViewAt(cid)->touched;
}

void Game::ObserveNeighbors(bool flag) {
  if (flag) {
    _SYS_vectors.neighborEvents.add = onNeighbor;
    _SYS_vectors.neighborEvents.remove = onNeighbor;
  } else {
    _SYS_vectors.neighborEvents.add = 0;
    _SYS_vectors.neighborEvents.remove = 0;
  }
}

//------------------------------------------------------------------
// BOOTSTRAP API
//------------------------------------------------------------------

void Game::MainLoop() {
  // reset everything
  mSimFrames = 0;
  mAnimFrames = 0;
  mIsDone = false;
  mNeedsSync = 0;
  mState.Init();
  mMap.Init();
  mPlayer.Init();
  for(ViewSlot* v = ViewBegin()+1; v!=ViewEnd(); ++v) { 
    if (v != mPlayer.View()->Parent()) {
      v->Init(); 
    }
  }

  // initial zoom out (yoinked and modded from TeleportTo)
  { 
    gChannelMusic.stop();
    PlaySfx(sfx_zoomIn);
    ViewSlot* view = mPlayer.View()->Parent();
    view->HideSprites();
    Vec2 room = mPlayer.Location();
    VidMode_BG2 vid(view->GetCube()->vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(mMap.Data()->tileset),
          mMap.GetTileId(room, Vec2(x, y))
        );
      }
    }
    vid.BG2_setBorder(0x0000);
    vid.set();
    for (float t = 1.0f; t > 0.0f; t -= 0.05f) {
      AffineMatrix m = AffineMatrix::identity();
      m.translate(64, 64);
      m.scale(1.f + 9.f*t);
      m.rotate(t * 1.1f);
      m.translate(-64, -64);
      vid.BG2_setMatrix(m);
      System::paint();
    }    
    System::paintSync();
    view->ShowLocation(mPlayer.Location());
    System::paintSync();
  }  
  (ViewBegin()+1)->ShowInventory();

  PlayMusic(music_castle);
  mSimTime = System::clock();
  ObserveNeighbors(true);
  CheckMapNeighbors();

  while(!mIsDone) {
    float dt = UpdateDeltaTime();
    if (sNeighborDirty) { 
      CheckMapNeighbors(); 
    }
    mPlayer.Update(dt);
    Paint();
    mSimFrames++;
  }
}

void Game::Paint(bool sync) {
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

float Game::UpdateDeltaTime() {
  float now = System::clock();
  float result = now - mSimTime;
  mSimTime = now;
  return result;
}

void Game::MovePlayerAndRedraw(int dx, int dy) {
  mPlayer.Move(dx, dy);
  mPlayer.UpdateAnimation(UpdateDeltaTime());
  mPlayer.View()->UpdatePlayer();
  System::paint();
}

void Game::WalkTo(Vec2 position) {
  PlaySfx(sfx_running);
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
  ViewSlot* view = (ViewSlot*)(mPlayer.View());
  Vec2 loc = mPlayer.Location();
  view->HideSprites();
  // blank other cubes
  for(ViewSlot* p = ViewBegin(); p != ViewEnd(); ++p) {
    if (p != view) { p->HideLocation(); }
  }
  // zoom in
  { 
    System::paintSync();
    PlaySfx(sfx_zoomOut);
    VidMode_BG2 vid(view->GetCube()->vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(mMap.Data()->tileset),
          mMap.GetTileId(loc, Vec2(x, y))
        );
      }
    }
    vid.BG2_setBorder(0x0000);
    vid.set();
    for (float t = 0; t < 1.0f; t += 0.05f) {
      AffineMatrix m = AffineMatrix::identity();
      m.translate(64, 64);
      m.scale(1.f+9.f*t);
      m.rotate(t * 1.1f);
      m.translate(-64, -64);
      vid.BG2_setMatrix(m);
      System::paint();
    }
  }
  mMap.SetData(m);
  for(ViewSlot* p = ViewBegin(); p!= ViewEnd(); ++p) {
    if (p != view) { p->Restore(); }
  }
  // zoom out
  { 
    PlaySfx(sfx_zoomIn);
    VidMode_BG2 vid(view->GetCube()->vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(mMap.Data()->tileset),
          mMap.GetTileId(room, Vec2(x, y))
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
  
  PlayMusic(mMap.Data() == &gMapData[1] ? music_dungeon : music_castle);

  // walk out of the in-gate
  Vec2 target = mMap.GetRoom(room)->Center(0);
  mPlayer.SetLocation(position, InferDirection(target - position));
  view->ShowLocation(room);
  WalkTo(target);
  CheckMapNeighbors();

  // clear out any accumulated time
  UpdateDeltaTime();
}

//------------------------------------------------------------------
// MISC EVENTS
//------------------------------------------------------------------

void Game::OnInventoryChanged() {
  for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->RefreshInventory();
  }
  const int firstSandwichId = 2;
  int count = 0;
  for(int i=firstSandwichId; i<firstSandwichId+4; ++i) {
    if(!mPlayer.HasItem(i)) {
      return;
    }
  }
  mIsDone = true;
}

//------------------------------------------------------------------
// NEIGHBOR HANDLING
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
  VisitMapView(visited, mPlayer.View()->Parent(), mPlayer.View()->GetRoom()->Location());
  for(ViewSlot* v = ViewBegin(); v!=ViewEnd(); ++v) {
    if (!visited[v->GetCubeID()]) { 
      if (v->HideLocation()) {
        PlaySfx(sfx_deNeighbor);
      }
    }
  }
  sNeighborDirty = false;
}
