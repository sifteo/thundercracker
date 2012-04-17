#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"


void Game::OnTick() {
  for(Bomb* p=mMap.BombBegin(); p!=mMap.BombEnd(); ++p) {
    if (p->FuseLit()) {
      p->UpdateFuse();
      if (!p->FuseLit()) { OnYesOhMyGodExplosion(*p); }
    }
  }
}

void Game::OnActiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasGateway() && !pRoom->OnEdge()) {
    OnEnterGateway(pRoom->Gateway());
  } else if (pRoom->HasNPC()) {
    const auto& npc = pRoom->NPC();
    if (npc.optional) { OnNpcChatter(npc); }
  }  else if (pRoom->HasSwitch()) {
    OnToggleSwitch(pRoom->Switch());
  }
}

unsigned Game::OnPassiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasItem()) {
    OnPickup(*pRoom);
  } else if (pRoom->HasTrapdoor()) {
    OnTrapdoor(*pRoom);
    return TRIGGER_RESULT_PATH_INTERRUPTED;
  } else if (pRoom->HasNPC()) {
    const auto& npc = pRoom->NPC();
    if (!npc.optional) { OnNpcChatter(npc); }
  }
  return TRIGGER_RESULT_NONE;
}

void Game::OnYesOhMyGodExplosion(Bomb& bomb) {
  RoomView* view;
  bool wasLocked = false;
  if (bomb.Item() == mPlayer.Equipment()) {
    mPlayer.CurrentView()->GetRoom().BombThisFucker();
    mPlayer.SetEquipment(0);
    view = mPlayer.CurrentView();
    view->HideEquip();
  } else {
    auto p = ListLockedViews();
    while(p.MoveNext()) {
      auto& room = p->GetRoomView().GetRoom();
      if (room.HasItem() && (&room.Item() == bomb.Item())) {
        room.ClearTrigger();
        room.BombThisFucker();
        view = &(p->GetRoomView());
        view->Unlock();
        view->HideItem();
        break;
      }
    }
  }
  auto& g = view->Parent().Canvas();
  view->HideOverlay();
  view->Parent().HideSprites();
  // flash white
  g.bg1.fillMask(vec(40,40)>>3, Explosion.tileSize());
  unsigned ef = 0;
  for(int rt=7, rb=8; rt>=0||rb<16; --rt, ++rb) {
    if (rt>=0) { g.bg0.span(vec(0, rt), 16, WhiteTile.tile(0)); }
    if (rb<16) { g.bg0.span(vec(0, rb), 16, WhiteTile); }
    if (ef%3 == 0) { g.bg1.image(vec(40,40)>>3, Explosion, ef/3); }
    ++ef;
    DoPaint();
  }
  // repaint room and quake
  view->Parent().DrawRoom(view->Id());
  auto deadline = SystemTime::now() + 2.333f;
  while(deadline.inFuture()) {
    float t = deadline - SystemTime::now();
    g.bg0.setPanning(vec(
      5.f * t * cos(16.f * t),
      5.f * t * sin(16.f * 2.1f*t)
    ));
    if (ef%3 == 0) {
      auto frm = ef/3;
      if (frm == Explosion.numFrames()) {
        g.bg1.erase();
      } else if (frm < Explosion.numFrames()) {
        g.bg1.image(vec(40,40)>>3, Explosion, frm);
      }
    }
    ++ef;
    DoPaint();
  }
  view->Restore();
  DoWait(1.f);
  CheckMapNeighbors();
}

void Game::OnToggleSwitch(const SwitchData& pSwitch) {
  RestorePearlIdle();
  for(unsigned i=1; i<=7; ++i) { // magic
    mPlayer.CurrentView()->DrawTrapdoorFrame(i);
    Paint();
    Paint();
  }
  OnTriggerEvent(pSwitch.eventType, pSwitch.eventId);
  for(int i=6; i>=0; --i) { // magic
    mPlayer.CurrentView()->DrawTrapdoorFrame(i);
    Paint();
    Paint();
  }
}

void Game::OnTrapdoor(Room& room) {
  //-------------------------------------------------------------------------
  // PLAYER TRIGGERED TRAPDOOR
  // animate the tiles opening
  for(unsigned i=1; i<=7; ++i) { // magic
    mPlayer.CurrentView()->DrawTrapdoorFrame(i);
    Paint();
    Paint();
  }
  // animate pearl falling TODO
  mPlayer.CurrentView()->HidePlayer();
  for(unsigned i=0; i<16; ++i) {
    Paint();
  }
  // animate the tiles closing
  for(int i=6; i>=0; --i) { // magic
    mPlayer.CurrentView()->DrawTrapdoorFrame(i);
    Paint();
    Paint();
  }
  // pan to respawn point
  ScrollTo(room.Trapdoor().respawnRoomId);

  // fall
  auto &view = mPlayer.CurrentView()->Parent();
  int animHeights[] = { 48, 32, 16, 0, 8, 12, 16, 12, 8, 0 };
  for(unsigned i=0; i<arraysize(animHeights); ++i) {
    view.GetRoomView().DrawPlayerFalling(animHeights[i]);
    Paint();
  }
  auto& targetRoom = mMap.GetRoom(room.Trapdoor().respawnRoomId);
  mPlayer.SetPosition(targetRoom.Center(0));
  mPlayer.SetDirection(BOTTOM);
  view.ShowLocation(mPlayer.Position()/128, true);
  CheckMapNeighbors();
  Paint();
}

void Game::OnInventoryChanged() {
  auto p = ListViews();
  while(p.MoveNext()) {
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


void Game::OnPickup(Room& room) {
  const auto& item = room.Item();
  const auto&itemType = gItemTypeData[item.itemId];
  if (itemType.storageType == STORAGE_EQUIPMENT) {

    //---------------------------------------------------------------------------
    // PLAYER TRIGGERED EQUIP PICKUP
    room.ClearTrigger();
    mPlayer.CurrentView()->HideItem();
    if (mPlayer.Equipment()) {
      OnDropEquipment(room);
    }
    PlaySfx(sfx_pickup);
    mPlayer.SetEquipment(&item);
    // do a pickup animation
    for(unsigned frame=0; frame<PlayerPickup.numFrames(); ++frame) {
      mPlayer.CurrentView()->SetPlayerImage(PlayerPickup, frame);
      SystemTime t=SystemTime::now();
      Paint();
      SystemTime now;
      do {
        now = SystemTime::now();
        // this calc is kinda annoyingly complex
        float u = float(now - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.numFrames();
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetEquipPosition(vec(0.f, -float(ITEM_OFFSET) * u) );
      } while(now-t<0.075f);
    }
    RestorePearlIdle();
    DescriptionDialog(
      "ITEM DISCOVERED", 
      itemType.description, 
      mPlayer.CurrentView()->Parent()
    );
    if (gItemTypeData[item.itemId].triggerType == ITEM_TRIGGER_BOMB) {
      Bomb* p = mMap.BombFor(item);
      ASSERT(p);
      p->OnPickup();
      mPlayer.CurrentView()->Unlock();
    }
  } else {
    //-----------------------------------------------------------------------
    // PLAYER TRIGGERED ITEM OR KEY PICKUP
    if (mState.FlagTrigger(item.trigger)) { room.ClearTrigger(); }
    if (mState.PickupItem(item.itemId)) {
      PlaySfx(sfx_pickup);
      OnInventoryChanged();
    }
    // do a pickup animation
    for(unsigned frame=0; frame<PlayerPickup.numFrames(); ++frame) {
      mPlayer.CurrentView()->SetPlayerImage(PlayerPickup, frame);
      SystemTime t=SystemTime::now();
      Paint();
      SystemTime now;
      do {
        // this calc is kinda annoyingly complex
        now = SystemTime::now();
        float u = float(now - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.numFrames();
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetItemPosition(vec(0.f, -36.f * u) );
      } while(now-t<0.075f);
    }
    RestorePearlIdle();
    DescriptionDialog(
      "ITEM DISCOVERED", 
      itemType.description, 
      mPlayer.CurrentView()->Parent()
    );
    mPlayer.CurrentView()->HideItem();        
  }

  OnTriggerEvent(item.trigger);
}

void Game::OnEnterGateway(const GatewayData& gate) {
  //---------------------------------------------------------------------------
  // PLAYER TRIGGERED GATEWAY
  const auto& targetMap = gMapData[gate.targetMap];
  if (mState.FlagTrigger(gate.trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
  WalkTo(128 * mPlayer.GetRoom()->Location() + vec<int>(gate.x, gate.y));
  mPlayer.SetEquipment(0);
  if (gate.targetType == TARGET_TYPE_GATEWAY) {
    const auto& pTargetGate = targetMap.gates[gate.targetId];
    TeleportTo(gMapData[gate.targetMap], vec(
      128 * (pTargetGate.trigger.room % targetMap.width) + pTargetGate.x,
      128 * (pTargetGate.trigger.room / targetMap.width) + pTargetGate.y
    ));
  } else {
    const auto& targetRoom = targetMap.rooms[gate.targetId];
    TeleportTo(gMapData[gate.targetMap], vec(
      128 * (gate.targetId % targetMap.width) + (targetRoom.centerX<<4),
      128 * (gate.targetId / targetMap.width) + (targetRoom.centerX<<4)
    ));
  }

  OnTriggerEvent(gate.trigger);
  RestorePearlIdle();
}

void Game::OnNpcChatter(const NpcData& npc) {
  //-------------------------------------------------------------------------
  // PLAYER TRIGGERED NPC DIALOG
  mPlayer.SetStatus(PLAYER_STATUS_IDLE);
  mPlayer.CurrentView()->UpdatePlayer();
  for(int i=0; i<16; ++i) { Paint(); }
  if (mState.FlagTrigger(npc.trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
  NpcDialog(gDialogData[npc.dialog], mPlayer.CurrentView()->Parent());
  DoPaint();
  OnTriggerEvent(npc.trigger);
  mPlayer.CurrentView()->Parent().Restore();
  RestorePearlIdle();
  DoPaint();
}

void Game::OnDropEquipment(Room& room) {
  const ItemData *pItem = mPlayer.Equipment();
  if (room.HasDepot()) {
    if (room.HasDepotContents()) {
      // no!
      mPlayer.CurrentView()->StartShake();
    } else {
      // place the item in the depot
      for(int frame=PlayerPickup.numFrames()-1; frame>0; --frame) {
        mPlayer.CurrentView()->SetPlayerImage(PlayerPickup, frame);
        Wait(0.075f, false);
      }
      mPlayer.SetEquipment(0);
      room.SetDepotContents(pItem);
      mPlayer.CurrentView()->StartNod();
      mPlayer.CurrentView()->HideEquip();
      mPlayer.CurrentView()->RefreshDepot();
      mPlayer.CurrentView()->SetPlayerImage(PlayerPickup);
      Wait(0.075f, false);
      // check if event should fire
      const auto& map = mMap.Data();
      const auto& group = map.depotGroups[room.Depot().group];
      unsigned count = 0;
      for(const DepotData* p=map.depots; p->room != 0xff; ++p) {
        count += (p->group == room.Depot().group && mMap.GetRoom(p->room).HasDepotContents());
      }
      if (count == group.count) {
        while(mPlayer.CurrentView()->IsWobbly()) { Paint(); }
        Wait(0.5f, false);
        OnTriggerEvent(group.eventType, group.eventId);
      }
    }
  } else {
    // ensure this isn't a bomb site
    bool isRespawnRoom = false;
    for(auto b=mMap.BombBegin(); b!=mMap.BombEnd(); ++b) {
      if (&b->RespawnRoom() == &room) {
        isRespawnRoom = true;
        break;
      }
    }
    if (isRespawnRoom) {
      mPlayer.CurrentView()->StartShake();
    } else {
      // show the dropped item as a normal trigger
      for(int frame=PlayerPickup.numFrames()-1; frame>0; --frame) {
        mPlayer.CurrentView()->SetPlayerImage(PlayerPickup, frame);
        Wait(0.075f, false);
      }
      mPlayer.SetEquipment(0);
      room.SetTrigger(TRIGGER_ITEM, &pItem->trigger);
      mPlayer.CurrentView()->HideEquip();
      mPlayer.CurrentView()->ShowItem(pItem);
      mPlayer.CurrentView()->SetPlayerImage(PlayerPickup);
      mPlayer.CurrentView()->StartNod();

      if (gItemTypeData[pItem->itemId].triggerType == ITEM_TRIGGER_BOMB) {
        mPlayer.CurrentView()->Lock();
      }

      Wait(0.075f, false);
    }

  }
}

void Game::OnUseEquipment() {
  // TODO: special cases for different types of equipment
  auto pRoom = mPlayer.GetRoom();
  if (pRoom->HasItem()) {
    OnPickup(*pRoom);
  } else {
    OnDropEquipment(*pRoom); // default
  }
  
}

bool Game::OnTriggerEvent(unsigned type, unsigned id) {
  switch(type) {
    case EVENT_ADVANCE_QUEST_AND_REFRESH: {
      mState.AdvanceQuest();
      mMap.RefreshTriggers();
      Viewport::Iterator p = ListViews();
      while(p.MoveNext()) {
        if (p->ShowingRoom()) {
          p->Restore();
        }
      }
      break;
    }
    case EVENT_ADVANCE_QUEST_AND_TELEPORT: {
      mState.AdvanceQuest();
      const QuestData* quest = mState.Quest();
      const MapData& map = gMapData[quest->mapId];
      const RoomData& room = map.rooms[quest->roomId];
      TeleportTo(map, vec<int> (
        128 * (quest->roomId % map.width) + 16 * room.centerX,
        128 * (quest->roomId / map.width) + 16 * room.centerY
      ));
      break;
    }
    case EVENT_OPEN_DOOR: {
      const bool needsPan = true;
      const DoorData& door = mMap.Data().doors[id];
      if (mState.IsActive(door.trigger)) {
        mState.FlagTrigger(door.trigger);
        //Room* targetRoom = mMap.GetRoom(door.trigger.room);
        bool didRestore = false;
        Viewport::Iterator p = ListViews();
        while(p.MoveNext()) {
          if (p->ShowingRoom() && p->GetRoomView().Id() == door.trigger.room) {
            p->GetRoomView().Restore();
            RoomNod(*p);
            didRestore = true;
            break;
          }
        }
        if (!didRestore) {
          ScrollTo(door.trigger.room);
          Wait(0.5f);
          mPlayer.CurrentView()->Parent().ShowLocation(mMap.GetLocation(door.trigger.room), true);
          Wait(0.5f);
          IrisOut(mPlayer.CurrentView()->Parent());
          mPlayer.CurrentView()->Parent().ShowLocation(mPlayer.Position()/128, true);
          Slide(mPlayer.CurrentView()->Parent());
          CheckMapNeighbors();
          Paint();
        }
        break;
      }
    }
    default: {
      return false;
    }
  }
  return true;
}

bool Game::TryEncounterBlock(Sokoblock& block) {
  const Int2 dir = BroadDirection();
  const Int2 loc_src = mPlayer.TargetRoom()->Location();
  const Int2 loc_dst = loc_src + dir;
  // no moving blocks off the map
  if (!mMap.Contains(loc_dst)) { return false; }
  Room& room = mMap.GetRoom(loc_dst);
  // no moving blocks into subdivided rooms
  if (room.IsSubdivided()) { return false; }
  // no moving blocks through walls or small portals
  Side dst_enter_side = LEFT;
  if (dir.x < 0) { dst_enter_side = RIGHT; }
  else if (dir.y > 0) { dst_enter_side = TOP; }
  else if (dir.y < 0) { dst_enter_side = BOTTOM; }
  if (room.CountOpenTilesAlongSide(dst_enter_side) < 4) {
    return false;
  }
  // no moving blocks onto other blocks
  for (Sokoblock* p=mMap.BlockBegin(); p!=mMap.BlockEnd(); ++p) {
    if (p != &block && room.IsShowingBlock(p)) {
      return false;
    }
  }
  return true;
}

bool Game::TryEncounterLava(Side dir) {
  ASSERT(0 <= dir && dir < 4);
  const Int2 baseTile = mPlayer.Position() >> 4;
  switch(dir) {
    case TOP: {
      const unsigned tid = mMap.GetGlobalTileId(baseTile - (vec<int>(1,1)));
      return mMap.IsTileLava(tid) || mMap.IsTileLava(tid+1);
    }
    case LEFT:
      return mMap.IsTileLava(mMap.GetGlobalTileId(baseTile - vec<int>(2,0)));
    case BOTTOM: {
      const unsigned tid = mMap.GetGlobalTileId(baseTile + vec<int>(-1,1));
      return mMap.IsTileLava(tid) || mMap.IsTileLava(tid+1);
    }
    default: // RIGHT
      return mMap.IsTileLava(mMap.GetGlobalTileId(baseTile + vec<int>(1,0)));
  }
}

