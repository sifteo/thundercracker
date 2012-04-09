#pragma once
#include "ViewSlot.h"
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


  bool mNeighborDirty;
  #if PLAYTESTING_HACKS
  static float sShakeTime;
  #endif

  ViewSlot mViews[NUM_CUBES];
  GameState mState;
  Map mMap;
  Player mPlayer;
  unsigned mAnimFrames;
  BroadPath mPath;
  NarrowPath mMoves;
  uint8_t mNeedsSync;
  uint8_t mIsDone;

public:

  // getters
  GameState* GetState() { return &mState; }
  Map* GetMap() { return &mMap; }
  Player* GetPlayer() { return &mPlayer; }
  ViewSlot* ViewAt(int i) { return mViews+i; }
  ViewSlot* ViewBegin() { return mViews; }
  ViewSlot* ViewEnd() { return mViews+NUM_CUBES; }
  ViewSlot::Iterator Views() { return ViewSlot::Iterator(0xffffffff); }
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
  void Paint(bool sync=false);
  void DoPaint(bool sync);
  void NeedsSync() { mNeedsSync = 1; }

  // events
  void OnNeighborAdd(RoomView* v1, Cube::Side s1, RoomView* v2, Cube::Side s2);
  void OnNeighborRemove(RoomView* v1, Cube::Side s1, RoomView* v2, Cube::Side s2);

private:

  static void onNeighbor(void *context, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);

  // cutscenes
  Cube* IntroCutscene();
  void WinScreen();


  // helpers
  bool AnyViewsTouched();
  void CheckMapNeighbors();
  void WalkTo(Int2 position, bool dosfx=true);
  void MovePlayerAndRedraw(int dx, int dy);
  int MovePlayerOneTile(Cube::Side dir, int progress, Sokoblock *blockToPush=0);
  void MoveBlock(Sokoblock* block, Int2 u);
  void TeleportTo(const MapData& m, Int2 position);
  void IrisOut(ViewSlot* view);
  void Zoom(ViewSlot* view, int roomId);
  void Slide(ViewSlot* view);
  void DescriptionDialog(const char* hdr, const char* msg, ViewSlot *view);
  void NpcDialog(const DialogData& data, ViewSlot *view);
  void RestorePearlIdle();
  void ScrollTo(unsigned roomId); // see impl for notes on how to "clean up" after this call :P
  void Wait(float seconds, bool touchToSkip=false);
  void RoomNod(ViewSlot* view);
  void RoomShake(ViewSlot* view);

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
  bool TryEncounterLava(Cube::Side dir);


};

extern Game gGame;

