#pragma once
#include "Viewport.h"
#include "Player.h"
#include "Map.h"
#include "GameState.h"

#define TRIGGER_RESULT_NONE             0
#define TRIGGER_RESULT_PATH_INTERRUPTED 1

class Game {
private:
  // hack because these aren't POD
  static SystemTime mPrevTime;
  static TimeDelta mDt;


  Viewport mViews[NUM_CUBES];
  bool mNeighborDirty;
  GameState mState;
  Map mMap;
  Player mPlayer;
  unsigned mAnimFrames;
  BroadPath mPath;
  NarrowPath mMoves;
  uint8_t mIsDone;

  uint32_t mActiveViewMask;
  uint32_t mLockedViewMask;
  uint32_t mTouchMask;

public:

  static Game* Inst();

  // getters
  GameState* GetState() { return &mState; }
  Map* GetMap() { return &mMap; }
  Player* GetPlayer() { return &mPlayer; }
  
  unsigned AnimFrame() const { return mAnimFrames; }
  Int2 BroadDirection() {
    ASSERT(mPlayer.Target()->view);
    return mPlayer.TargetRoom()->Location() - mPlayer.CurrentRoom()->Location();
  }
  SystemTime LastPaintTime() const { return mPrevTime; }
  TimeDelta Dt() const { return mDt; }

  bool ShowingMinimap() const { 
      return false; 
  }

  // methods  
  void MainLoop();
  void Paint();
  void DoPaint();

  // events
  
  // listing views
  Viewport& ViewAt(int i) { return mViews[i]; }

  Viewport::Iterator ListViews(uint32_t mask=0xffffffff) { 
    return Viewport::Iterator(mask & mActiveViewMask); 
  }

  Viewport::Iterator ListLockedViews() { return ListViews(mLockedViewMask); }
  Viewport::Iterator ListTouchedViews() { return ListViews(mTouchMask); }
  void OnViewLocked(RoomView* pRoom) { mLockedViewMask |= pRoom->Parent()->GetMask(); }
  void OnViewUnlocked(RoomView* pRoom) { mLockedViewMask &= ~pRoom->Parent()->GetMask(); }
  void UnlockAllViews();

  struct {
    Viewport::Iterator begin() { return ++Game::Inst()->ListViews(); }
    Viewport::Iterator end() { return Viewport::Iterator(); }
  } views;

private:

  void OnNeighbor(unsigned c0, unsigned s0, unsigned c1, unsigned s1);
  void OnTouch(unsigned cube);
  void CheckMapNeighbors();
  void CheckTouches();

  // cutscenes
  Viewport* IntroCutscene();
  //void WinScreen();


  // helpers
  bool AnyViewsTouched();
  void WalkTo(Int2 position, bool dosfx=true);
  void MovePlayerAndRedraw(int dx, int dy);
  int MovePlayerOneTile(Side dir, int progress, Sokoblock *blockToPush=0);
  void MoveBlock(Sokoblock* block, Int2 u);
  void TeleportTo(const MapData& m, Int2 position);
  void IrisOut(Viewport* view);
  void Zoom(Viewport* view, int roomId);
  void Slide(Viewport* view);
  void DescriptionDialog(const char* hdr, const char* msg, Viewport *view);
  void NpcDialog(const DialogData& data, Viewport *view);
  void RestorePearlIdle();
  void ScrollTo(unsigned roomId); // see impl for notes on how to "clean up" after this call :P
  void Wait(float seconds, bool touchToSkip=false);
  void RoomNod(Viewport* view);
  void RoomShake(Viewport* view);

  // events
  void OnTick();
  unsigned OnPassiveTrigger();
  void OnActiveTrigger();
  void OnYesOhMyGodExplosion(Bomb* p);
  void OnInventoryChanged();
  void OnTrapdoor(Room *pRoom);
  void OnToggleSwitch(const SwitchData* pSwitch);
  void OnPickup(Room *pRoom);
  void OnDropEquipment(Room *pRoom);
  void OnUseEquipment();
  void OnEnterGateway(const GatewayData* pGate);
  void OnNpcChatter(const NpcData* pNpc);
  bool OnTriggerEvent(const TriggerData& trig) { return OnTriggerEvent(trig.eventType, trig.eventId); }
  bool OnTriggerEvent(unsigned type, unsigned id);

  bool TryEncounterBlock(Sokoblock* block);
  bool TryEncounterLava(Side dir);


};

extern Game gGame;

