#pragma once
#include "ViewSlot.h"
#include "Player.h"
#include "Map.h"
#include "GameState.h"

class Game {
private:
  ViewSlot mViews[NUM_CUBES];
  GameState mState;
  Map mMap;
  Player mPlayer;
  float mSimTime;
  float mDeltaTime;
  unsigned mSimFrames;
  unsigned mAnimFrames;
  unsigned mNeedsSync;
  BroadPath mPath;
  NarrowPath mMoves;
  bool mIsDone;

public:

  // getters
  inline GameState* GetState() { return &mState; }
  inline Map* GetMap() { return &mMap; }
  inline Player* GetPlayer() { return &mPlayer; }
  inline ViewSlot* ViewAt(int i) { return mViews+i; }
  inline ViewSlot* ViewBegin() { return mViews; }
  inline ViewSlot* ViewEnd() { return mViews+NUM_CUBES; }
  inline float SimTime() const { return mSimTime; }
  inline unsigned SimFrame() const { return mSimFrames; }
  inline unsigned AnimFrame() const { return mAnimFrames; }

  // methods  
  void MainLoop(Cube* pPrimary);
  void Update(bool sync=false);
  void Paint(bool sync=false);
  void NeedsSync() { mNeedsSync = 1; }

  // events
  void OnNeighborAdd(RoomView* v1, Cube::Side s1, RoomView* v2, Cube::Side s2);
  void OnNeighborRemove(RoomView* v1, Cube::Side s1, RoomView* v2, Cube::Side s2);

private:

  // helpers
  float UpdateDeltaTime();
  void ObserveNeighbors(bool flag);
  void CheckMapNeighbors();

  void WalkTo(Vec2 position, bool dosfx=true);
  void MovePlayerAndRedraw(int dx, int dy);

  void TeleportTo(const MapData& m, Vec2 position);
  void IrisOut(ViewSlot* view);
  void Zoom(ViewSlot* view, int roomId);

  unsigned OnPassiveTrigger();
  void OnActiveTrigger();
  void OnInventoryChanged();

};

extern Game* pGame;

