#pragma once
#include "Common.h"
#include "Content.h"
#include "Sokoblock.h"

#define USERDATA_NONE     0
#define USERDATA_TRIGGER  1
#define USERDATA_SUBDIV   2
#define USERDATA_PROP     3
// MAX COUNT = 16

#define PROP_UNDEFINED    0
#define PROP_TRAPDOOR     1
#define PROP_DEPOT        2

class Room {
private:
  const void* mUserdata; // pointer to a trigger or subdivision or trapdoor
  
  const void* mOtherdata; // this pointer is overloaded to show either the door (default)
                          // or the depot contents (if USERDATA_PROP|PROP_DEPOT is set)
  
  uint16_t mOverlayIndex;
  uint8_t mOverlayTile;
  uint8_t mUserdataType : 4;
  uint8_t mInnerType : 4;

public:

  //---------------------------------------------------------------------------
  // generic methods
  //---------------------------------------------------------------------------

  unsigned Id() const;
  Int2 Location() const;
  const RoomData* Data() const;
  inline Int2 Position() const { return 128 * Location(); }
  Int2 LocalCenter(unsigned subdiv) const;
  inline Int2 Center(unsigned subdiv) const { return Position() + 16 * LocalCenter(subdiv); }
  inline bool HasUserdata() const { return mUserdataType; }
  void Clear();
  bool IsShowingBlock(const Sokoblock* pBlock);  
  unsigned CountOpenTilesAlongSide(Cube::Side side);

  //---------------------------------------------------------------------------
  // triggers
  //---------------------------------------------------------------------------

  int TriggerType() const { return mUserdataType == USERDATA_TRIGGER ? mInnerType : 0; }
  bool HasTrigger() const { return mUserdataType == USERDATA_TRIGGER && mInnerType; }
  bool HasGateway() const { return mUserdataType == USERDATA_TRIGGER && mInnerType == TRIGGER_GATEWAY; }
  bool HasItem() const { return mUserdataType == USERDATA_TRIGGER && mInnerType == TRIGGER_ITEM; }
  bool HasNPC() const { return mUserdataType == USERDATA_TRIGGER && mInnerType == TRIGGER_NPC; }
  const GatewayData* TriggerAsGate() const { ASSERT(HasGateway());  return (const GatewayData*) mUserdata; }
  const ItemData* TriggerAsItem() const { ASSERT(HasItem()); return (const ItemData*) mUserdata; }
  const NpcData* TriggerAsNPC() const { ASSERT(HasNPC()); return (const NpcData*) mUserdata; }

  void SetTrigger(int type, const TriggerData* p) { 
    mUserdataType = USERDATA_TRIGGER;
    mInnerType = type; 
    mUserdata = p; 
  }
  
  void ClearTrigger() { 
    ASSERT(mUserdataType == USERDATA_TRIGGER);
    mInnerType = TRIGGER_UNDEFINED; 
  }
  

  //---------------------------------------------------------------------------
  // subdivs
  //---------------------------------------------------------------------------

  bool IsSubdivided() const { return mUserdataType == USERDATA_SUBDIV && mInnerType; }
  int SubdivType() const { return mUserdataType == USERDATA_SUBDIV ? mInnerType : 0; }

  bool IsBridge() const { 
    return mUserdataType == USERDATA_SUBDIV && (
      mInnerType == SUBDIV_BRDG_VER || mInnerType == SUBDIV_BRDG_HOR
    ); 
  }
  bool IsDiag() const {
    return mUserdataType == USERDATA_SUBDIV && (
      mInnerType == SUBDIV_DIAG_POS || mInnerType == SUBDIV_DIAG_NEG
    ); 
  }

  const DiagonalSubdivisionData* SubdivAsDiagonal() const { ASSERT(IsDiag()); return (const DiagonalSubdivisionData*)mUserdata; }
  const BridgeSubdivisionData* SubdivAsBridge() const { ASSERT(IsBridge()); return (const BridgeSubdivisionData*)mUserdata;}

  void SetDiagonalSubdivision(const DiagonalSubdivisionData* diag);
  void SetBridgeSubdivision(const BridgeSubdivisionData* bridge);

  //---------------------------------------------------------------------------
  // props
  //---------------------------------------------------------------------------

  inline bool HasTrapdoor() const { return mUserdataType == USERDATA_PROP && mInnerType == PROP_TRAPDOOR; }
  inline bool HasDepot() const { return mUserdataType == USERDATA_PROP && mInnerType == PROP_DEPOT; }

  inline void SetTrapdoor(const TrapdoorData* trapDoorData) {
    mUserdataType = USERDATA_PROP;
    mInnerType = PROP_TRAPDOOR;
    mUserdata = trapDoorData;
  }

  inline void SetDepot(const DepotData* depot) {
    ASSERT(mOtherdata == 0);
    mUserdataType == USERDATA_PROP;
    mInnerType == PROP_TRAPDOOR;
    mUserdata = depot;
  }

  const TrapdoorData* Trapdoor() const { ASSERT(HasTrapdoor()); return (const TrapdoorData*) mUserdata; }
  const DepotData* Depot() const { ASSERT(HasDepot()); return (const DepotData*) mUserdata; }

  bool HasDepotContents() const { ASSERT(HasDepot()); return mOtherdata != 0; }
  void SetDepotContents(const ItemData* item) { ASSERT(HasDepot()); mOtherdata = item; }
  const ItemData* DepotContents() const { ASSERT(HasDepot()); return (const ItemData*) mOtherdata; }

  //---------------------------------------------------------------------------
  // doors
  //---------------------------------------------------------------------------

  bool HasDoor() const { return !HasDepot() && mOtherdata != 0; }
  bool HasOpenDoor() const;
  bool HasClosedDoor() const;
  const DoorData* Door() const { ASSERT(HasDoor()); return (const DoorData*) mOtherdata; }
  void SetDoor(const DoorData* p) { mOtherdata = p; }
  bool OpenDoor();

  //---------------------------------------------------------------------------
  // overlays
  //---------------------------------------------------------------------------

  bool HasOverlay() const { return mOverlayIndex != 0xffff; }
  const uint8_t* OverlayBegin() const;
  unsigned OverlayTile() const { return mOverlayTile; }
  void SetOverlay(uint16_t rleIndex, uint8_t firstTile) { mOverlayIndex = rleIndex; mOverlayTile = firstTile; }
};
