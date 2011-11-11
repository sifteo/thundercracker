#pragma once
#include "Base.h"

class Enemy;
struct MapRoom;

#define ROOM_NONE (Vec2(-1,-1))
#define PLAYER_SPRITE_ID 0
#define ENEMY_SPRITE_ID 1
#define ITEM_SPRITE_ID 2

class GameView {
public:
  Cube cube;
  int visited;

private:
  Vec2 mRoom;
  int mSpriteCount; 
  
public:  
  GameView();
  
  // getters
  bool IsShowingRoom() const;
  bool InSpriteMode() const;
  Vec2 Room() const { return mRoom; }
  MapRoom* GetMapRoom() const;
  Cube::Side VirtualTiltDirection() const;
  GameView* VirtualNeighborAt(Cube::Side side) const;
  
  // methods
  void Init();
  void ShowRoom(Vec2 room);
  void ClearRoom();

  void RestoreSimpleCanvas();
  
  void ShowPlayer();
  void UpdatePlayer();
  void HidePlayer();
  
  void ShowEnemy(Enemy* pEnemy);
  void UpdateEnemy(Enemy* pEnemy);
  void HideEnemy(Enemy* pEnemy);
  
private:
  bool EnterSpriteMode();
  void SetSpriteImage(int id, int tile);
  void HideSprite(int id);
  void ResizeSprite(int id, int px, int py);
  void MoveSprite(int id, int px, int py);
  bool ExitSpriteMode();
  
  void DrawBackground();
  void DoEnterSpriteMode();
  void DoEnterSimpleMode();

};
