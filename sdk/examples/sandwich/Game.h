#pragma once
#include "GameView.h"
#include "Player.h"
#include "Map.h"
#include "GameState.h"

class Game {
public:
  GameView views[NUM_CUBES];
  GameState state;
  Player player;
  Map map;
  float mSimTime;
  int mNeedsSync;
  bool mIsDone;

public:

  // Getters
  
  GameView* ViewAt(int i) { return views+i; }
  GameView* ViewBegin() { return views; }
  GameView* ViewEnd() { return views+NUM_CUBES; }
  
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

