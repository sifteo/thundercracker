#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"


void Game::OnActiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasGateway()) {
    OnEnterGateway(pRoom->Gateway());
  } else if (pRoom->HasNPC()) {
    const NpcData* npc = pRoom->NPC();
    if (npc->optional) { OnNpcChatter(npc); }
  }  else if (pRoom->HasSwitch()) {
    OnToggleSwitch(pRoom->Switch());
  }
}

unsigned Game::OnPassiveTrigger() {
  Room* pRoom = mPlayer.GetRoom();
  if (pRoom->HasItem()) {
    OnPickup(pRoom);
  } else if (pRoom->HasTrapdoor()) {
    OnTrapdoor(pRoom);
    return TRIGGER_RESULT_PATH_INTERRUPTED;
  } else if (pRoom->HasNPC()) {
    const NpcData* npc = pRoom->NPC();
    if (!npc->optional) { OnNpcChatter(npc); }
  }
  return TRIGGER_RESULT_NONE;
}

void Game::OnToggleSwitch(const SwitchData* pSwitch) {
  RestorePearlIdle();
  for(unsigned i=1; i<=7; ++i) { // magic
    mPlayer.CurrentView()->DrawTrapdoorFrame(i);
    Paint(true);
    Paint(true);
  }
  OnTriggerEvent(pSwitch->eventType, pSwitch->eventId);
  for(int i=6; i>=0; --i) { // magic
    mPlayer.CurrentView()->DrawTrapdoorFrame(i);
    Paint(true);
    Paint(true);
  }
}

void Game::OnTrapdoor(Room *pRoom) {
  //-------------------------------------------------------------------------
  // PLAYER TRIGGERED TRAPDOOR
  // animate the tiles opening
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
  ScrollTo(pRoom->Trapdoor()->respawnRoomId);

  // fall
  ViewSlot *pView = mPlayer.CurrentView()->Parent();
  int animHeights[] = { 48, 32, 16, 0, 8, 12, 16, 12, 8, 0 };
  for(unsigned i=0; i<arraysize(animHeights); ++i) {
    pView->GetRoomView()->DrawPlayerFalling(animHeights[i]);
    Paint(true);
  }
  const Room* targetRoom = mMap.GetRoom(pRoom->Trapdoor()->respawnRoomId);
  mPlayer.SetPosition(targetRoom->Center(0));
  mPlayer.SetDirection(SIDE_BOTTOM);
  pView->ShowLocation(mPlayer.Position()/128, true);
  CheckMapNeighbors();
  Paint(true);
}

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
  const ItemData* pItem = pRoom->Item();
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
      SystemTime t=SystemTime::now();
      Paint();
      do {
        // this calc is kinda annoyingly complex
        float u = float(mSimTime - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.frames;
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetEquipPosition(Vec2(0.f, -float(ITEM_OFFSET) * u) );
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
      SystemTime t=SystemTime::now();
      Paint();
      do {
        // this calc is kinda annoyingly complex
        float u = float(mSimTime - t) / 0.075f;
        const float du = 1.f / (float) PlayerPickup.frames;
        u = (frame + u) * du;
        u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
        Paint();
        mPlayer.CurrentView()->SetItemPosition(Vec2(0.f, -36.f * u) );
      } while(SystemTime::now()-t<0.075f);
    }
    mPlayer.CurrentView()->SetPlayerFrame(PlayerStand.index+ (SIDE_BOTTOM<<4));
    DescriptionDialog(
      "ITEM DISCOVERED", 
      itemType.description, 
      mPlayer.CurrentView()->Parent()
    );
    mPlayer.CurrentView()->HideItem();        
  }

  OnTriggerEvent(pItem->trigger);
}

void Game::OnEnterGateway(const GatewayData* pGate) {
  //---------------------------------------------------------------------------
  // PLAYER TRIGGERED GATEWAY
  const MapData& targetMap = gMapData[pGate->targetMap];
  if (mState.FlagTrigger(pGate->trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
  WalkTo(128 * mPlayer.GetRoom()->Location() + Vec2<int>(pGate->x, pGate->y));
  mPlayer.SetEquipment(0);
  if (pGate->targetType == TARGET_TYPE_GATEWAY) {
    const GatewayData& pTargetGate = targetMap.gates[pGate->targetId];
    TeleportTo(gMapData[pGate->targetMap], Vec2(
      128 * (pTargetGate.trigger.room % targetMap.width) + pTargetGate.x,
      128 * (pTargetGate.trigger.room / targetMap.width) + pTargetGate.y
    ));
  } else {
    const RoomData& targetRoom = targetMap.rooms[pGate->targetId];
    TeleportTo(gMapData[pGate->targetMap], Vec2(
      128 * (pGate->targetId % targetMap.width) + (targetRoom.centerX<<4),
      128 * (pGate->targetId / targetMap.width) + (targetRoom.centerX<<4)
    ));
  }

  OnTriggerEvent(pGate->trigger);
  RestorePearlIdle();
}

void Game::OnNpcChatter(const NpcData* pNpc) {
  //-------------------------------------------------------------------------
  // PLAYER TRIGGERED NPC DIALOG
  mPlayer.SetStatus(PLAYER_STATUS_IDLE);
  mPlayer.CurrentView()->UpdatePlayer();
  for(int i=0; i<16; ++i) { Paint(true); }
  if (mState.FlagTrigger(pNpc->trigger)) { mPlayer.GetRoom()->ClearTrigger(); }
  NpcDialog(gDialogData[pNpc->dialog], mPlayer.CurrentView()->Parent());
  System::paintSync();
  OnTriggerEvent(pNpc->trigger);
  mPlayer.CurrentView()->Parent()->Restore(false);
  RestorePearlIdle();
  System::paintSync();
}

void Game::OnDropEquipment(Room *pRoom) {
  const ItemData *pItem = mPlayer.Equipment();
  if (pRoom->HasDepot()) {
    if (pRoom->HasDepotContents()) {
      // no!
      mPlayer.CurrentView()->StartShake();
    } else {
      // place the item in the depot
      for(int frame=PlayerPickup.frames-1; frame>0; --frame) {
        mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index + (frame<<4));
        Wait(0.075f, false);
      }
      mPlayer.SetEquipment(0);
      pRoom->SetDepotContents(pItem);
      mPlayer.CurrentView()->StartNod();
      mPlayer.CurrentView()->HideEquip();
      mPlayer.CurrentView()->RefreshDepot();
      mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index);
      Wait(0.075f, false);
      // check if event should fire
      const MapData& map = *mMap.Data();
      const DepotGroupData& group = map.depotGroups[pRoom->Depot()->group];
      unsigned count = 0;
      for(const DepotData* p=map.depots; p->room != 0xff; ++p) {
        count += (p->group == pRoom->Depot()->group && mMap.GetRoom(p->room)->HasDepotContents());
      }
      if (count == group.count) {
        while(mPlayer.CurrentView()->IsWobbly()) { Paint(); }
        Wait(0.5f, false);
        OnTriggerEvent(group.eventType, group.eventId);
      }
    }
  } else {
    // show the dropped item as a normal trigger
    for(int frame=PlayerPickup.frames-1; frame>0; --frame) {
      mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index + (frame<<4));
      Wait(0.075f, false);
    }
    mPlayer.SetEquipment(0);
    pRoom->SetTrigger(TRIGGER_ITEM, &pItem->trigger);
    mPlayer.CurrentView()->HideEquip();
    mPlayer.CurrentView()->ShowItem(pItem);
    mPlayer.CurrentView()->SetPlayerFrame(PlayerPickup.index);
    Wait(0.075f, false);
  }
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

bool Game::OnTriggerEvent(unsigned type, unsigned id) {
  switch(type) {
    case EVENT_ADVANCE_QUEST_AND_REFRESH:
      mState.AdvanceQuest();
      mMap.RefreshTriggers();
      for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
        if (p->IsShowingRoom()) {
          p->Restore(false);
        }
      }
      break;
    case EVENT_ADVANCE_QUEST_AND_TELEPORT: {
      mState.AdvanceQuest();
      const QuestData* quest = mState.Quest();
      const MapData& map = gMapData[quest->mapId];
      const RoomData& room = map.rooms[quest->roomId];
      TeleportTo(map, Vec2<int> (
        128 * (quest->roomId % map.width) + 16 * room.centerX,
        128 * (quest->roomId / map.width) + 16 * room.centerY
      ));
      break;
    }
    case EVENT_OPEN_DOOR: {
      const bool needsPan = true;
      const DoorData& door = mMap.Data()->doors[id];
      if (mState.IsActive(door.trigger)) {
        mState.FlagTrigger(door.trigger);
        //Room* targetRoom = mMap.GetRoom(door.trigger.room);
        bool didRestore = false;
        for(ViewSlot *p = ViewBegin(); p!=ViewEnd(); ++p) {
          if (p->IsShowingRoom() && p->GetRoomView()->Id() == door.trigger.room) {
            p->GetRoomView()->Restore();
            RoomNod(p);
            didRestore = true;
            break;
          }
        }
        if (!didRestore) {
          ScrollTo(door.trigger.room);
          Wait(0.5f);
          mPlayer.CurrentView()->Parent()->ShowLocation(mMap.GetLocation(door.trigger.room), true, true);
          Wait(0.5f);
          IrisOut(mPlayer.CurrentView()->Parent());
          mPlayer.CurrentView()->Parent()->ShowLocation(mPlayer.Position()/128, true, false);
          Slide(mPlayer.CurrentView()->Parent());
          CheckMapNeighbors();
          Paint(true);
        }
        break;
      }
    }
    default: return false;
  }
  return true;
}

bool Game::TryEncounterBlock(Sokoblock* block) {
  const Int2 dir = BroadDirection();
  const Int2 loc_src = mPlayer.TargetRoom()->Location();
  const Int2 loc_dst = loc_src + dir;
  // no moving blocks off the map
  if (!mMap.Contains(loc_dst)) { return false; }
  Room* pRoom = mMap.GetRoom(loc_dst);
  // no moving blocks into subdivided rooms
  if (pRoom->IsSubdivided()) { return false; }
  // no moving blocks through walls or small portals
  Cube::Side dst_enter_side = SIDE_LEFT;
  if (dir.x < 0) { dst_enter_side = SIDE_RIGHT; }
  else if (dir.y > 0) { dst_enter_side = SIDE_TOP; }
  else if (dir.y < 0) { dst_enter_side = SIDE_BOTTOM; }
  if (pRoom->CountOpenTilesAlongSide(dst_enter_side) < 4) {
    return false;
  }
  // no moving blocks onto other blocks
  for (Sokoblock* p=mMap.BlockBegin(); p!=mMap.BlockEnd(); ++p) {
    if (p != block && pRoom->IsShowingBlock(p)) {
      return false;
    }
  }
  return true;
}

bool Game::TryEncounterLava(Cube::Side dir) {
  ASSERT(0 <= dir && dir < 4);
  const Int2 baseTile = mPlayer.Position() >> 4;
  switch(dir) {
    case SIDE_TOP: {
      const unsigned tid = mMap.GetGlobalTileId(baseTile - (Vec2<int>(1,1)));
      return mMap.IsTileLava(tid) || mMap.IsTileLava(tid+1);
    }
    case SIDE_LEFT:
      return mMap.IsTileLava(mMap.GetGlobalTileId(baseTile - Vec2<int>(2,0)));
    case SIDE_BOTTOM: {
      const unsigned tid = mMap.GetGlobalTileId(baseTile + Vec2<int>(-1,1));
      return mMap.IsTileLava(tid) || mMap.IsTileLava(tid+1);
    }
    default: // SIDE_RIGHT
      return mMap.IsTileLava(mMap.GetGlobalTileId(baseTile + Vec2<int>(1,0)));
  }
}

