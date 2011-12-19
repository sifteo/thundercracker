#pragma once
#include "GameView.h"
#include "Player.h"
#include "Map.h"
#include "Enemy.h"

#define NUM_ENEMIES 8

class Game {
public:
  GameView views[NUM_CUBES];
  Player player;
  Map map;
  Enemy enemies[NUM_ENEMIES];
  float mSimTime;
  
public:

  // Getters
  
  Cube* CubeAt(int i) { return &(views[i].cube); }
  GameView* ViewAt(int i) { return views+i; }
  GameView* ViewBegin() { return views; }
  GameView* ViewEnd() { return views+NUM_CUBES; }
  Enemy* EnemyBegin() { return enemies; }
  Enemy* EnemyEnd() { return enemies+NUM_ENEMIES; }
  
  void MainLoop();
  
  // Trigger Actions
  void WalkTo(Vec2 position);
  void TeleportTo(const MapData& m, Vec2 position);

  void OnNeighborAdd(GameView* v1, Cube::Side s1, GameView* v2, Cube::Side s2);
  void OnNeighborRemove(GameView* v1, Cube::Side s1, GameView* v2, Cube::Side s2);
  

private:

  float UpdateDeltaTime();

  void ObserveNeighbors(bool flag);
  void CheckMapNeighbors();
  void MovePlayerAndRedraw(int dx, int dy);
};

extern Game* pGame;

