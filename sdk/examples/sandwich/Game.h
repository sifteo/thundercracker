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
  int mNeedsSync;
  bool mIsDone;

public:

  Game();

  // Getters
  
  inline GameState* GetState() { return &mState; }
  inline Map* GetMap() { return &mMap; }
  inline Player* GetPlayer() { return &mPlayer; }

  GameView* ViewAt(int i) { return mViews+i; }
  GameView* ViewBegin() { return mViews; }
  GameView* ViewEnd() { return mViews+NUM_CUBES; }
  
  void MainLoop();
  void Paint(bool sync=false);

  // Trigger Actions
  void WalkTo(Vec2 position);
  void TeleportTo(const MapData& m, Vec2 position);
  float SimTime() const { return mSimTime; }

  void OnNeighborAdd(GameView* v1, Cube::Side s1, GameView* v2, Cube::Side s2);
  void OnNeighborRemove(GameView* v1, Cube::Side s1, GameView* v2, Cube::Side s2);
  
  void OnInventoryChanged();

  void NeedsSync() { mNeedsSync = 1; }

private:

  float UpdateDeltaTime();

  void ObserveNeighbors(bool flag);
  void CheckMapNeighbors();
  void MovePlayerAndRedraw(int dx, int dy);
};

extern Game* pGame;

