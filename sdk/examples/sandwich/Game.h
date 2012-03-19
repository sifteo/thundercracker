#pragma once
#include "ViewSlot.h"
#include "Player.h"
#include "Map.h"
#include "GameState.h"

#define TRIGGER_RESULT_NONE             0
#define TRIGGER_RESULT_PATH_INTERRUPTED 1

class Game {
private:
  static bool sNeighborDirty;
  #if PLAYTESTING_HACKS
  static float sShakeTime;
  #endif


  ViewSlot mViews[NUM_CUBES];
  GameState mState;
  Map mMap;
  Player mPlayer;
  float mSimTime;
  unsigned mAnimFrames;
  BroadPath mPath;
  NarrowPath mMoves;
  uint8_t mNeedsSync;
  uint8_t mIsDone;

public:

  // getters
  inline GameState* GetState() { return &mState; }
  inline Map* GetMap() { return &mMap; }
  inline Player* GetPlayer() { return &mPlayer; }
  inline ViewSlot* ViewAt(int i) { return mViews+i; }
  inline ViewSlot* ViewBegin() { return mViews; }
  inline ViewSlot* ViewEnd() { return mViews+NUM_CUBES; }
  inline unsigned AnimFrame() const { return mAnimFrames; }
  inline Vec2 BroadDirection() {
    ASSERT(mPlayer.Target()->view);
    return mPlayer.TargetRoom()->Location() - mPlayer.CurrentRoom()->Location();
  }

  bool ShowingMinimap() const { 
      return false; 
  }

  // methods  
  void MainLoop();
  void Paint(bool sync=false);
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
  void CheckMapNeighbors();

  void WalkTo(Vec2 position, bool dosfx=true);
  void MovePlayerAndRedraw(int dx, int dy);
  int MovePlayerOneTile(Cube::Side dir, int progress, Sokoblock *blockToPush=0);
  void MoveBlock(Sokoblock* block, Vec2 u);
  void TeleportTo(const MapData& m, Vec2 position);
  void IrisOut(ViewSlot* view);
  void Zoom(ViewSlot* view, int roomId);
  void DescriptionDialog(const char* hdr, const char* msg, ViewSlot *view);
  void NpcDialog(const DialogData& data, ViewSlot *view);
  

  unsigned OnPassiveTrigger();
  void OnActiveTrigger();
  void OnInventoryChanged();

  void OnPickup(Room *pRoom);
  void OnDropEquipment(Room *pRoom);
  void OnUseEquipment();
  void OnEnterGateway(Room *pRoom);
  void OnNpcChatter(Room *pRoom);
  void OnTriggerEvent(unsigned id);

  bool OnEncounterBlock(Sokoblock* block);

  void RestorePearlIdle();


};

extern Game gGame;

