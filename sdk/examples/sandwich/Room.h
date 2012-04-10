#pragma once
#include "Common.h"
#include "Content.h"
#include "Sokoblock.h"

#define PRIMARY_NONE     0
#define PRIMARY_TRIGGER  1
#define PRIMARY_SUBDIV   2
#define PRIMARY_PROP     3
// MAX COUNT = 16

#define SECONDARY_UNDEFINED    0
#define SECONDARY_TRAPDOOR     1
#define SECONDARY_DEPOT        2
#define SECONDARY_SWITCH       3

class Room {
private:
  const void* mPrimarySlot;
  const void* mSecondarySlot;
  
  uint16_t mOverlayIndex;
  uint8_t mOverlayTile;
  
  uint8_t mPrimarySlotType : 4;
  uint8_t mPrimarySlotId : 4;

  struct {
    uint8_t canMask : 4;
    uint8_t didMask : 4;
  } mBomb;

public:

  //---------------------------------------------------------------------------
  // generic methods
  //---------------------------------------------------------------------------

  unsigned Id() const;
  Int2 Location() const;
  const RoomData* Data() const;
  Int2 Position() const { return 128 * Location(); }
  Int2 LocalCenter(unsigned subdiv) const;
  Int2 Center(unsigned subdiv) const { return Position() + 16 * LocalCenter(subdiv); }
  bool HasUserdata() const { return mPrimarySlotType; }
  void Clear();
  bool IsShowingBlock(const Sokoblock* pBlock);  
  unsigned CountOpenTilesAlongSide(Cube::Side side);

  //---------------------------------------------------------------------------
  // bombypoos
  //---------------------------------------------------------------------------

  bool CanBomb(Cube::Side s) const { ASSERT(0<=s && s<4); return (mBomb.canMask & (1<<s)) != 0; }
  bool DidBomb(Cube::Side s) const { ASSERT(0<=s && s<4); return (mBomb.didMask & (1<<s)) != 0; }
  bool HasBombedSides() const { return mBomb.didMask != 0; }
  void SetCanBomb(Cube::Side s) { ASSERT(0<=s && s<4); mBomb.canMask |= (1<<s); }
  void SetDidBomb(Cube::Side s) { mBomb.didMask |= ((1<<s) & mBomb.canMask); }
  void BombThisFucker();

  //---------------------------------------------------------------------------
  // triggers
  //---------------------------------------------------------------------------

  int TriggerType() const { return mPrimarySlotType == PRIMARY_TRIGGER ? mPrimarySlotId : 0; }
  bool HasTrigger() const { return mPrimarySlotType == PRIMARY_TRIGGER && mPrimarySlotId; }
  bool HasGateway() const { return mPrimarySlotType == PRIMARY_TRIGGER && mPrimarySlotId == TRIGGER_GATEWAY; }
  bool HasItem() const { return mPrimarySlotType == PRIMARY_TRIGGER && mPrimarySlotId == TRIGGER_ITEM; }
  bool HasNPC() const { return mPrimarySlotType == PRIMARY_TRIGGER && mPrimarySlotId == TRIGGER_NPC; }
  const GatewayData* Gateway() const { ASSERT(HasGateway());  return (const GatewayData*) mPrimarySlot; }
  const ItemData* Item() const { ASSERT(HasItem()); return (const ItemData*) mPrimarySlot; }
  const NpcData* NPC() const { ASSERT(HasNPC()); return (const NpcData*) mPrimarySlot; }

  void SetTrigger(int type, const TriggerData* p) { 
    ASSERT(!mPrimarySlot);
    mPrimarySlotType = PRIMARY_TRIGGER;
    mPrimarySlotId = type; 
    mPrimarySlot = p; 
  }
  
  void ClearTrigger() { 
    ASSERT(mPrimarySlotType == PRIMARY_TRIGGER);
    mPrimarySlotId = TRIGGER_UNDEFINED; 
    mPrimarySlot = 0;
  }

  //---------------------------------------------------------------------------
  // subdivs
  //---------------------------------------------------------------------------

  bool IsSubdivided() const { return mPrimarySlotType == PRIMARY_SUBDIV && mPrimarySlotId; }
  int SubdivType() const { return mPrimarySlotType == PRIMARY_SUBDIV ? mPrimarySlotId : 0; }

  bool IsBridge() const { 
    return mPrimarySlotType == PRIMARY_SUBDIV && (
      mPrimarySlotId == SUBDIV_BRDG_VER || mPrimarySlotId == SUBDIV_BRDG_HOR
    ); 
  }
  bool IsDiag() const {
    return mPrimarySlotType == PRIMARY_SUBDIV && (
      mPrimarySlotId == SUBDIV_DIAG_POS || mPrimarySlotId == SUBDIV_DIAG_NEG
    ); 
  }

  const DiagonalSubdivisionData* SubdivAsDiagonal() const { ASSERT(IsDiag()); return (const DiagonalSubdivisionData*)mPrimarySlot; }
  const BridgeSubdivisionData* SubdivAsBridge() const { ASSERT(IsBridge()); return (const BridgeSubdivisionData*)mPrimarySlot;}

  void SetDiagonalSubdivision(const DiagonalSubdivisionData* diag);
  void SetBridgeSubdivision(const BridgeSubdivisionData* bridge);

  //---------------------------------------------------------------------------
  // props
  //---------------------------------------------------------------------------

  bool HasTrapdoor() const { return mPrimarySlotType == PRIMARY_PROP && mPrimarySlotId == SECONDARY_TRAPDOOR; }
  bool HasDepot() const { return mPrimarySlotType == PRIMARY_PROP && mPrimarySlotId == SECONDARY_DEPOT; }
  bool HasSwitch() const { return mPrimarySlotType == PRIMARY_PROP && mPrimarySlotId == SECONDARY_SWITCH; }
  const TrapdoorData* Trapdoor() const { ASSERT(HasTrapdoor()); return (const TrapdoorData*) mPrimarySlot; }
  const DepotData* Depot() const { ASSERT(HasDepot()); return (const DepotData*) mPrimarySlot; }
  const SwitchData* Switch() const { ASSERT(HasSwitch()); return (const SwitchData*) mPrimarySlot; }

  void SetTrapdoor(const TrapdoorData* trapDoorData) {
    ASSERT(!mPrimarySlot);
    mPrimarySlotType = PRIMARY_PROP;
    mPrimarySlotId = SECONDARY_TRAPDOOR;
    mPrimarySlot = trapDoorData;
  }

  void SetDepot(const DepotData* depot) {
    ASSERT(!mPrimarySlot);
    ASSERT(!mSecondarySlot);
    mPrimarySlotType = PRIMARY_PROP;
    mPrimarySlotId = SECONDARY_DEPOT;
    mPrimarySlot = depot;
  }

  void SetSwitch(const SwitchData* toggle) {
    ASSERT(!mPrimarySlot);
    mPrimarySlotType = PRIMARY_PROP;
    mPrimarySlotId = SECONDARY_SWITCH;
    mPrimarySlot = toggle;
  }

  bool HasDepotContents() const { return HasDepot() && mSecondarySlot != 0; }
  void SetDepotContents(const ItemData* item) { ASSERT(HasDepot()); mSecondarySlot = item; }
  const ItemData* DepotContents() const { ASSERT(HasDepot()); return (const ItemData*) mSecondarySlot; }

  //---------------------------------------------------------------------------
  // doors
  //---------------------------------------------------------------------------

  bool HasDoor() const { return !HasDepot() && mSecondarySlot != 0; }
  bool HasOpenDoor() const;
  bool HasClosedDoor() const;
  const DoorData* Door() const { ASSERT(HasDoor()); return (const DoorData*) mSecondarySlot; }
  void SetDoor(const DoorData* p) { ASSERT(!HasDepot()); mSecondarySlot = p; }
  bool OpenDoor();

  //---------------------------------------------------------------------------
  // overlays
  //---------------------------------------------------------------------------

  bool HasOverlay() const { return mOverlayIndex != 0xffff; }
  const uint8_t* OverlayBegin() const;
  unsigned OverlayTile() const { return mOverlayTile; }
  void SetOverlay(uint16_t rleIndex, uint8_t firstTile) { mOverlayIndex = rleIndex; mOverlayTile = firstTile; }
};
