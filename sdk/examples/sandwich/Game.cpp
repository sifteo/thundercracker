#include "Game.h"

static bool sNeighborDirty = false;

static void onNeighbor(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {
  sNeighborDirty = true;
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

Game gGame;

void Game::InitializeAssets() {
  for (Cube::ID i = 0; i < NUM_CUBES; i++) {
    CubeAt(i)->enable(i);
    CubeAt(i)->loadAssets(GameAssets);
    VidMode_BG0_ROM rom(CubeAt(i)->vbuf);
    rom.init();
    rom.BG0_text(Vec2(1,1), "Loading...");
  }
  for (;;) {
    bool done = true;
    for (Cube::ID i = 0; i < NUM_CUBES; i++) {
      VidMode_BG0_ROM rom(CubeAt(i)->vbuf);
      rom.BG0_progressBar(Vec2(0,7), CubeAt(i)->assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
      if (!CubeAt(i)->assetDone(GameAssets))
        done = false;
    }
    System::paint();
    if (done) {
      break;
		}
  }
  for(Cube::ID i=0; i<NUM_CUBES; ++i) {
    VidMode_BG0 mode(CubeAt(i)->vbuf);
    mode.init();
    mode.BG0_drawAsset(Vec2(0,0), Sting);
  }
  float time = System::clock();
  while(System::clock() - time < 1.f) {
    for(GameView*v = ViewBegin(); v!=ViewEnd(); ++v) {
      v->cube.vbuf.touch();
    }
    System::paint();
  }
}

void Game::MainLoop() {
  float simTime = System::clock();
  for(GameView* v = ViewBegin(); v!=ViewEnd(); ++v) {
    v->Init();
  }
  ObserveNeighbors(true);
  System::paint();

  while(1) {
    float now = System::clock();
    float dt = now - simTime;
    simTime = now;
    if (sNeighborDirty) { 
      CheckMapNeighbors(); 
    }
    player.Update(dt);
    for(Enemy* p = EnemyBegin(); p != EnemyEnd(); ++p) {
      p->Update(dt);
    }
    System::paint();
  }
}

void Game::MovePlayerAndRedraw(int dx, int dy) {
  player.Move(dx, dy);
  player.CurrentView()->UpdatePlayer();
  System::paint();
}

void Game::WalkTo(Vec2 position) {
  Vec2 delta = position - player.Position();
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
  Vec2 room = position/128;
  GameView* view = player.CurrentView();
  view->HidePlayer();
  // blank other cubes
  for(GameView* p = ViewBegin(); p != ViewEnd(); ++p) {
    if (p != view) { p->HideRoom(); }
  }
  // zoom in
  { 
    System::paintSync();
  
    VidMode_BG2 vid(view->cube.vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(map.Data()->tileset),
          map.Data()->GetTileId(view->Location(), Vec2(x, y))
        );
      }
    }
    vid.BG2_setBorder(0x0000);
    vid.set();
    for (float t = 0; t < 1.0f; t += 0.025f) {
      AffineMatrix m = AffineMatrix::identity();
      m.translate(64, 64);
      m.scale(1.f+9.f*t);
      m.rotate(t * 1.1f);
      m.translate(-64, -64);
      vid.BG2_setMatrix(m);
      System::paint();
    }
  }
  map.SetData(m);
  // zoom out
  { 
    VidMode_BG2 vid(view->cube.vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(map.Data()->tileset),
          map.Data()->GetTileId(room, Vec2(x, y))
        );
      }
    }
    for (float t = 1.0f; t > 0.0f; t -= 0.025f) {
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
  
  // walk out of the in-gate
  Vec2 target = 128*room + Vec2(64,64);
  player.SetLocation(position, InferDirection(target - position));
  view->Init();
  WalkTo(target);
  CheckMapNeighbors();
}

void Game::TakeItem() {
  int itemId = player.CurrentView()->Room()->itemId;
  if (itemId) {
    map.GetRoom(player.Location())->SetItem(0);
    
    // generalize to player.GetItem(itemId)?
    if (itemId == ITEM_BASIC_KEY) {
      player.IncrementBasicKeyCount();
    }

    // do a pickup animation
    for(unsigned frame=0; frame<PlayerPickup.frames; ++frame) {
      player.CurrentView()->SetPlayerFrame(PlayerPickup.index + (frame * PlayerPickup.width * PlayerPickup.height));
      
      for(float t=System::clock(); System::clock()-t<0.075f;) {
        // this calc is kinda annoyingly complex
        float u = (System::clock() - t) / 0.075f;
        float du = 1.f / (float) PlayerPickup.frames;
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);

        System::paint();
        player.CurrentView()->SetItemPosition(Vec2(0, -40.f * u) );
      }
    }
    player.CurrentView()->SetPlayerFrame(PlayerStand.index+ SIDE_BOTTOM* PlayerStand.width * PlayerStand.height);
    for(float t=System::clock(); System::clock()-t<0.25f;) { System::paint(); }
    player.CurrentView()->HideItem();
  }
}

//------------------------------------------------------------------
// NEIGHBOR HANDLING
//------------------------------------------------------------------

static void VisitMapView(GameView* view, Vec2 loc, GameView* origin=0) {
  if (!view || view->visited) { return; }
  view->visited = true;
  view->ShowLocation(loc);
  if (origin) {
    view->cube.orientTo(origin->cube);
  }
  for(Cube::Side i=0; i<NUM_SIDES; ++i) {
    VisitMapView(view->VirtualNeighborAt(i), loc+kSideToUnit[i], view);
  }
}

void Game::CheckMapNeighbors() {
  for(GameView* v = ViewBegin(); v!=ViewEnd(); ++v) {
    v->visited = false;
  }
  VisitMapView(player.KeyView(), player.KeyView()->Location());
  for(GameView* v = ViewBegin(); v!=ViewEnd(); ++v) {
    if (!v->visited) { v->HideRoom(); }
  }
  sNeighborDirty = false;
}
