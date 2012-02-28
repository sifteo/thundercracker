#pragma once
#include "Common.h"
#include "Content.h"

#define USERDATA_NONE 0
#define USERDATA_TRIGGER 1
#define USERDATA_SUBDIV 2
#define USERDATA_TRAPDOOR 3 // generalize to EVENT(?)/TRAPDOOR

class Room {
private:
  const void* mUserdata; // pointer to a trigger or subdivision or trapdoor
  const DoorData* mDoor;
  uint16_t mOverlayIndex;
  uint8_t mOverlayTile;
  uint8_t mUserdataType : 4;
  uint8_t mInnerType : 4;

public:

  //---------------------------------------------------------------------------
  // generic methods
  //---------------------------------------------------------------------------

  unsigned Id() const;
  Vec2 Location() const;
  const RoomData* Data() const;
  inline Vec2 Position() const { return 128 * Location(); }
  Vec2 LocalCenter(unsigned subdiv) const;
  inline Vec2 Center(unsigned subdiv) const { return Position() + 16 * LocalCenter(subdiv); }
  inline bool HasUserdata() const { return mUserdataType; }
  void Clear();

  //---------------------------------------------------------------------------
  // triggers
  //---------------------------------------------------------------------------

  inline int TriggerType() const { 
    return mUserdataType == USERDATA_TRIGGER ? mInnerType : 0; 
  }

  inline bool HasTrigger() const {
    return mUserdataType == USERDATA_TRIGGER && mInnerType; 
  }

  inline bool HasGateway() const { 
    return mUserdataType == USERDATA_TRIGGER && mInnerType == TRIGGER_GATEWAY; 
  }

  inline bool HasItem() const { 
    return mUserdataType == USERDATA_TRIGGER && mInnerType == TRIGGER_ITEM; 
  }

  inline bool HasNPC() const { 
    return mUserdataType == USERDATA_TRIGGER && mInnerType == TRIGGER_NPC; 
  }
  
  inline const GatewayData* TriggerAsGate() const { 
    ASSERT(mUserdataType == USERDATA_TRIGGER);
    ASSERT(mInnerType == TRIGGER_GATEWAY); 
    return (const GatewayData*) mUserdata; 
  }

  inline const ItemData* TriggerAsItem() const { 
    ASSERT(mUserdataType == USERDATA_TRIGGER);
    ASSERT(mInnerType == TRIGGER_ITEM); 
    return (const ItemData*) mUserdata; 
  }

  inline const NpcData* TriggerAsNPC() const { 
    ASSERT(mUserdataType == USERDATA_TRIGGER);
    ASSERT(mInnerType == TRIGGER_NPC); 
    return (const NpcData*) mUserdata; 
  }

  inline void SetTrigger(int type, const TriggerData* p) { 
    mUserdataType = USERDATA_TRIGGER;
    mInnerType = type; 
    mUserdata = p; 
  }
  
  inline void ClearTrigger() { 
    ASSERT(mUserdataType == USERDATA_TRIGGER);
    mInnerType = TRIGGER_UNDEFINED; 
  }
  

  //---------------------------------------------------------------------------
  // subdivs
  //---------------------------------------------------------------------------

  bool IsSubdivided() const { 
    return mUserdataType == USERDATA_SUBDIV && mInnerType; 
  }

  bool IsBridge() const { 
    return mUserdataType == USERDATA_SUBDIV && (
      mInnerType == SUBDIV_BRDG_VER || mInnerType == SUBDIV_BRDG_HOR
    ); 
  }

  int SubdivType() const { 
    return mUserdataType == USERDATA_SUBDIV ? mInnerType : 0; 
  }

  const DiagonalSubdivisionData* SubdivAsDiagonal() const { 
    ASSERT(mUserdataType == USERDATA_SUBDIV);
    ASSERT(mInnerType == SUBDIV_DIAG_POS || mInnerType == SUBDIV_DIAG_NEG);  
    return (const DiagonalSubdivisionData*)mUserdata; 
  }

  const BridgeSubdivisionData* SubdivAsBridge() const { 
    ASSERT(mUserdataType == USERDATA_SUBDIV);
    ASSERT(mInnerType == SUBDIV_BRDG_VER || mInnerType == SUBDIV_BRDG_HOR); 
    return (const BridgeSubdivisionData*)mUserdata; 
  }

  void SetDiagonalSubdivision(const DiagonalSubdivisionData* diag);
  void SetBridgeSubdivision(const BridgeSubdivisionData* bridge);

  //---------------------------------------------------------------------------
  // trapdoors
  //---------------------------------------------------------------------------

  inline bool HasTrapdoor() const {
    return mUserdataType == USERDATA_TRAPDOOR;
  }

  inline void SetTrapdoor(const TrapdoorData* trapDoorData) {
    mUserdataType = USERDATA_TRAPDOOR;
    mUserdata = trapDoorData;
  }

  const TrapdoorData* Trapdoor() const {
    ASSERT(mUserdataType == USERDATA_TRAPDOOR);
    return (const TrapdoorData*) mUserdata;
  }

  //---------------------------------------------------------------------------
  // doors
  //---------------------------------------------------------------------------

  bool HasDoor() const { return mDoor != 0; }
  bool HasOpenDoor() const;
  bool HasClosedDoor() const;
  void SetDoor(const DoorData* p) { mDoor = p; }
  bool OpenDoor();

  //---------------------------------------------------------------------------
  // overlays
  //---------------------------------------------------------------------------

  bool HasOverlay() const { return mOverlayIndex != 0xffff; }
  const uint8_t* OverlayBegin() const;
  unsigned OverlayTile() const { return mOverlayTile; }
  void SetOverlay(uint16_t rleIndex, uint8_t firstTile) { mOverlayIndex = rleIndex; mOverlayTile = firstTile; }
};
