#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

Game gGame;
SystemTime Game::mPrevTime;
TimeDelta Game::mDt(0.f);
#if PLAYTESTING_HACKS
float Game::sShakeTime = -1.f;
#endif


void Game::Paint(bool sync) {
  if (mNeighborDirty) { 
    CheckMapNeighbors(); 
  }
  SystemTime now = SystemTime::now();
  mPlayer.Update();
  ViewSlot::Iterator p = ListViews();
  while(p.MoveNext()) {
    p->Update();
  }
  DoPaint(sync);
  mAnimFrames++;

  #if PLAYTESTING_HACKS
    if (mPlayer.View()->GetCube()->isShaking()) {
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

void Game::DoPaint(bool sync) {
  if (sync || mNeedsSync) {
    System::paintSync();
    if (mNeedsSync > 0) {
      mNeedsSync--;
    }
  } else {
    System::paint();
  }
  SystemTime newTime = SystemTime::now();
  mDt = newTime - mPrevTime;
  mPrevTime = newTime;
  // New/Lost Cube Hook?
}

void Game::MoveBlock(Sokoblock* block, Int2 u) {
  if (block) {
    block->Move(u);
    mPlayer.TargetView()->UpdateBlock();

    // Could be optimized, perhaps
    const Cube::Side dir = InferDirection(u);
    ViewSlot* view = mPlayer.TargetView()->Parent()->VirtualNeighborAt(dir);
    if (view && view->ShowingRoom()) {
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

  const Int2 unit = kSideToUnit[dir];
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

void Game::WalkTo(Int2 position, bool dosfx) {
  if (dosfx) {
    PlaySfx(sfx_running);
  }
  Int2 delta = position - mPlayer.Position();
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

void Game::TeleportTo(const MapData& m, Int2 position) {
  #if MUSIC_ON
    gChannelMusic.stop();
  #endif
  Int2 room = position/128;
  ViewSlot* view = mPlayer.View();
  unsigned roomId = mPlayer.GetRoom()->Id();
  
  // blank other cubes
  ViewSlot::Iterator p = ListViews();
  while(p.MoveNext()) {
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
  Int2 target = mMap.GetRoom(room)->Center(0);
  mPlayer.SetDirection(InferDirection(target - position));
  view->ShowLocation(room, true);
  WalkTo(target, false);
  CheckMapNeighbors();
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
    DoPaint(true);
  }
}

void Game::Zoom(ViewSlot* view, int roomId) {
#if DO_ZOOM
  PlaySfx(sfx_zoomIn);
  DoPaint(true);
  VidMode_BG2 vid(view->GetCube()->vbuf);
  vid.init();
  vid.BG2_setBorder(0x0000);
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
    DoPaint(false);
  }    
  DoPaint(true);
#endif
}

void Game::ScrollTo(unsigned roomId) {
  // blank other cubes
  ViewSlot *pView = mPlayer.CurrentView()->Parent();
  ViewSlot::Iterator p = ListViews();
  while(p.MoveNext()) {
    if (p != pView) { p->HideLocation(false); }
  }
  // hide sprites and overlay
  pView->HideSprites();
  BG1Helper(*pView->GetCube()).Flush();
  Paint(true);

  const Int2 targetLoc = mMap.GetLocation(roomId);
  const Int2 currentLoc = mPlayer.GetRoom()->Location();
  const Int2 start = 128 * currentLoc;
  const Int2 delta = 128 * (targetLoc - currentLoc);
  const Int2 target = start + delta;
  Int2 pos;
  ViewMode mode = pView->Graphics();
  SystemTime t=SystemTime::now(); 
  do {
    float u = float(SystemTime::now()-t) / 2.333f;
    u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
    pos = Vec2(start.x + int(u * delta.x), start.y + int(u * delta.y));
    DrawOffsetMap(&mode, mMap.Data(), pos);
    Paint(true);
  } while(SystemTime::now()-t<2.333f && (pos-target).len2() > 4);
  mode.BG0_setPanning(Vec2(0,0));
  DrawRoom(&mode, mMap.Data(), roomId);
  Paint(true);
}

void Game::Slide(ViewSlot* view) {
  ViewMode g = view->Graphics();
  const int dt = 16;
  ASSERT(128%dt == 0);
  g.setWindow(128-dt,dt);
  Paint(true);
  for(unsigned i=dt; i<=128; i+=dt) {
    g.setWindow(128-i, i);
    Paint(false);
  }
}

bool Game::AnyViewsTouched() {
  ViewSlot::Iterator p = ListViews();
  while(p.MoveNext()) {
    if (p->Touched()) { return true; }
  }
  return false;
}

void Game::Wait(float seconds, bool touchToSkip) {
  for(SystemTime t=SystemTime::now(); SystemTime::now()-t<seconds;) { 
    Paint(); 
    if (touchToSkip && AnyViewsTouched()) {
      return;
    }
  }
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
      bg0_tiles[i] = vbuf.peek( mode.BG0_addr(Vec2(i%18, i/18)) );
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
        for (unsigned i = 0; i < 16; i ++) {
            view.SetAlpha(i<<4);
            gGame.Paint();
        }
        view.SetAlpha(255);
        gGame.Paint();
        Wait(5.f, true);
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
  Wait(4, true);
  pView->GetCube()->vbuf.touch();
  Paint(true);
  mPlayer.CurrentView()->Parent()->Restore();
  mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.index+ (SIDE_BOTTOM<<4));
  #if GFX_ARTIFACT_WORKAROUNDS    
    Paint(true);
    pView->GetCube()->vbuf.touch();
    Paint(true);
  #endif
}

void Game::RestorePearlIdle() {
  if (mPlayer.Direction() != SIDE_BOTTOM || mPlayer.Status() != PLAYER_STATUS_IDLE) {
    // Always look "south" after an action
    mPlayer.SetDirection(SIDE_BOTTOM);
    mPlayer.SetStatus(PLAYER_STATUS_IDLE);
    mPlayer.CurrentView()->UpdatePlayer();
  }
}

void Game::RoomNod(ViewSlot* view) {
  view->GetRoomView()->StartNod();
  while(view->ShowingRoom() && view->GetRoomView()->IsWobbly()) {
    Paint();
  }
}

void Game::RoomShake(ViewSlot* view) {
  view->GetRoomView()->StartShake();
  while(view->ShowingRoom() && view->GetRoomView()->IsWobbly()) {
    Paint();
  }
}

