#pragma once
#include "GameView.h"
#include "Player.h"
#include "Map.h"
#include "GameState.h"

class Game {
private:
  GameView mViews[NUM_CUBES];
  GameState mState;
  Map mMap;
  Player mPlayer;
  float mSimTime;
  unsigned mSimFrames;
  unsigned mAnimFrames;
  unsigned mNeedsSync;
  bool mIsDone;

public:

  // getters
  inline GameState* GetState() { return &mState; }
  inline Map* GetMap() { return &mMap; }
  inline Player* GetPlayer() { return &mPlayer; }
  inline GameView* ViewAt(int i) { return mViews+i; }
  inline GameView* ViewBegin() { return mViews; }
  inline GameView* ViewEnd() { return mViews+NUM_CUBES; }
  inline float SimTime() const { return mSimTime; }
  inline unsigned SimFrame() const { return mSimFrames; }
  inline unsigned AnimFrame() const { return mAnimFrames; }

  // methods  
  void MainLoop();
  void Paint(bool sync=false);
  void NeedsSync() { mNeedsSync = 1; }
  void WalkTo(Vec2 position);
  void TeleportTo(const MapData& m, Vec2 position);

  // events
  void OnNeighborAdd(GameView* v1, Cube::Side s1, GameView* v2, Cube::Side s2);
  void OnNeighborRemove(GameView* v1, Cube::Side s1, GameView* v2, Cube::Side s2);
  void OnInventoryChanged();

private:

  // helpers
  float UpdateDeltaTime();
  void ObserveNeighbors(bool flag);
  void CheckMapNeighbors();
  void MovePlayerAndRedraw(int dx, int dy);
};

extern Game* pGame;

