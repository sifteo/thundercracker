#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

Game gGame;
SystemTime Game::mPrevTime;
TimeDelta Game::mDt(0.f);
#if PLAYTESTING_HACKS
float Game::sShakeTime = -1.f;
#endif

Game* Game::Inst() {
  return &gGame;
}

void Game::Paint(bool sync) {
  if (mNeighborDirty) { 
    CheckMapNeighbors(); 
  }
  SystemTime now = SystemTime::now();
  mPlayer.Update();
  for(Viewport& view : views) {
    view.Update();
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
    System::paint();
    System::finish();
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

void Game::UnlockAllViews() {
  auto i = ListLockedViews();
  while(i.MoveNext()) {
    i->GetRoomView()->Unlock();
  }
  mLockedViewMask = 0;
}

void Game::MoveBlock(Sokoblock* block, Int2 u) {
  if (block) {
    block->Move(u);
    mPlayer.TargetView()->UpdateBlock();

    // Could be optimized, perhaps
    const Side dir = InferDirection(u);
    Viewport* view = mPlayer.TargetView()->Parent()->VirtualNeighborAt(dir);
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


int Game::MovePlayerOneTile(Side dir, int progress, Sokoblock* block) {
  mPlayer.SetDirection(dir);

  const Int2 unit = Int2::unit(dir);
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
  mPlayer.SetDirection(InferDirection(vec(dx, dy)));
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
  Viewport* view = mPlayer.View();
  unsigned roomId = mPlayer.GetRoom()->Id();
  
  // blank other cubes
  Viewport::Iterator p = ListViews();
  while(p.MoveNext()) {
    if (p != view) { p->HideLocation(); }
  }

  IrisOut(view);
  mMap.SetData(m);
  mPlayer.SetPosition(position);
  if (pMinimap) { pMinimap->Restore(); }
  Zoom(view, room.x + room.y * mMap.Data()->width);
  
  VideoBuffer& g = view->Video();

  // todo: expose music in level editor?
  PlayMusic(mMap.Data()->tileset == &TileSet_dungeon ? music_dungeon : music_castle);

  // walk out of the in-gate
  Int2 target = mMap.GetRoom(room)->Center(0);
  mPlayer.SetDirection(InferDirection(target - position));
  view->ShowLocation(room, true);
  WalkTo(target, false);
  UnlockAllViews();
  CheckMapNeighbors();
}

void Game::IrisOut(Viewport* view) {
  view->HideSprites();
  view->Video().bg1.eraseMask();
  VideoBuffer& mode = view->Video();
  for(unsigned i=0; i<8; ++i) {
    for(unsigned x=i; x<16-i; ++x) {
      mode.bg0.image(vec(x, i), BlackTile);
      mode.bg0.image(vec(x, 16-i-1), BlackTile);
    }
    for(unsigned y=i+1; y<16-i-1; ++y) {
      mode.bg0.image(vec(i, y), BlackTile);
      mode.bg0.image(vec(16-i-1, y), BlackTile);
    }
    DoPaint(false);
  }
  for(unsigned i=0; i<8; ++i) {
    DoPaint(true);
  }
}

void Game::Zoom(Viewport* view, int roomId) {
#if DO_ZOOM
  PlaySfx(sfx_zoomIn);
  DoPaint(true);
  VidMode_BG2 vid(view->GetCube()->vbuf);
  vid.init();
  vid.BG2_setBorder(0x0000);
  for(int x=0; x<8; ++x) {
    for(int y=0; y<8; ++y) {
      vid.BG2_drawAsset(
        vec(x<<1,y<<1),
        *(mMap.Data()->tileset),
        mMap.GetTileId(roomId, vec(x, y))
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
  Viewport *pView = mPlayer.CurrentView()->Parent();
  Viewport::Iterator p = ListViews();
  while(p.MoveNext()) {
    if (p != pView) { p->HideLocation(false); }
  }
  // hide sprites and overlay
  pView->HideSprites();
  VideoBuffer& mode = pView->Video();
  mode.bg1.eraseMask();
  Paint(true);

  const Int2 targetLoc = mMap.GetLocation(roomId);
  const Int2 currentLoc = mPlayer.GetRoom()->Location();
  const Int2 start = 128 * currentLoc;
  const Int2 delta = 128 * (targetLoc - currentLoc);
  const Int2 target = start + delta;
  Int2 pos;
  SystemTime t=SystemTime::now(); 
  do {
    float u = float(SystemTime::now()-t) / 2.333f;
    u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
    pos = vec(start.x + int(u * delta.x), start.y + int(u * delta.y));
    DrawOffsetMap(pView, mMap.Data(), pos);
    Paint(true);
  } while(SystemTime::now()-t<2.333f && (pos-target).len2() > 4);
  mode.bg0.setPanning(vec(0,0));
  DrawRoom(pView, mMap.Data(), roomId);
  Paint(true);
}

void Game::Slide(Viewport* view) {
  VideoBuffer& g = view->Video();
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
  Viewport::Iterator p = ListViews();
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

void Game::NpcDialog(const DialogData& data, Viewport *vslot) {
    Dialog view;
    VideoBuffer& mode = vslot->Video();
    PlaySfx(sfx_neighbor);
    for(unsigned i=0; i<8; ++i) { mode.sprites[i].hide(); }
    mode.bg0.image(vec(0,10), DialogBox);

    // TODO?
    // save BG0 (above dialog line)
    //VideoBuffer& vbuf = vslot->Video();
    //uint16_t bg0_tiles[180];
    //for(unsigned i=0; i<180; ++i) {
    //  bg0_tiles[i] = vbuf.peek( mode.BG0_addr(vec(i%18, i/18)) );
    //}

    for(unsigned line=0; line<data.lineCount; ++line) {
        const DialogTextData& txt = data.lines[line];
        if (line == 0 || data.lines[line-1].detail != txt.detail) {
          // TODO
          //if (line > 0) {
          //  Paint(true);
          //  mode.setWindow(0, 80);
          //  _SYS_vbuf_write(&vbuf.sys, mode.BG0_addr(vec(0,0)), bg0_tiles, 180);
          //}
          //BG1Helper ovrly(*vslot->GetCube());
          //ovrly.DrawAsset(vec(txt.detail == &NPC_Detail_pearl_detail ? 1 : 2, 0), *(txt.detail));
          //ovrly.Flush();
          //Paint(true);
          //Now set up a letterboxed 128x48 mode
          mode.setWindow(80, 48);
          view.Init(&vslot->Video());
        }
        view.Erase();
        Paint(true);
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

void Game::DescriptionDialog(const char* hdr, const char* msg, Viewport* pView) {
  DoPaint(true);
  VideoBuffer& gfx = pView->Video();
  gfx.setWindow(80+16,128-80-16);
  Dialog view;
  view.Init(&pView->Video());
  view.Erase();
  if (hdr) { view.Show(hdr); }
  view.ShowAll(msg);
  Paint(true);
  for(int t=0; t<16; t++) {
    gfx.setWindow(80+15-(t),128-80-15+(t));
    view.SetAlpha(t<<4);
    Paint();
  }
  view.SetAlpha(255);
  Wait(4, true);
  Paint(true);
  mPlayer.CurrentView()->Parent()->Restore();
  mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.tile(0) + (BOTTOM<<4));
  DoPaint(true);
}

void Game::RestorePearlIdle() {
  if (mPlayer.Direction() != BOTTOM || mPlayer.Status() != PLAYER_STATUS_IDLE) {
    // Always look "south" after an action
    mPlayer.SetDirection(BOTTOM);
    mPlayer.SetStatus(PLAYER_STATUS_IDLE);
    mPlayer.CurrentView()->UpdatePlayer();
  }
}

void Game::RoomNod(Viewport* view) {
  view->GetRoomView()->StartNod();
  while(view->ShowingRoom() && view->GetRoomView()->IsWobbly()) {
    Paint();
  }
}

void Game::RoomShake(Viewport* view) {
  view->GetRoomView()->StartShake();
  while(view->ShowingRoom() && view->GetRoomView()->IsWobbly()) {
    Paint();
  }
}

