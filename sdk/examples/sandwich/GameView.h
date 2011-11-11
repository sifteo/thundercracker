#pragma once
#include "Base.h"

class Enemy;
struct MapRoom;

class GameView {
public:
  Cube cube;
  int visited;

private:
  Vec2 mRoom;
  
public:  
  GameView();
  
  // getters
  bool IsShowingRoom() const;
  bool InSpriteMode() const;
  Vec2 Location() const { return mRoom; }
  MapRoom* Room() const;
  Cube::Side VirtualTiltDirection() const;
  GameView* VirtualNeighborAt(Cube::Side side) const;
  
  // methods
  void Init();

  void ShowLocation(Vec2 room);
  void HideRoom();
  
  void ShowPlayer();
  void UpdatePlayer();
  void HidePlayer();
  
  void ShowEnemy(Enemy* pEnemy);
  void UpdateEnemy(Enemy* pEnemy);
  void HideEnemy(Enemy* pEnemy);
  
  void ShowItem(int itemId);
  void SetItemPosition(Vec2 p);
  void HideItem();

private:  
  // sprite manipulation methods
  void EnterSpriteMode();
  void SetSpriteImage(int id, int tile);
  void HideSprite(int id);
  void ResizeSprite(int id, int px, int py);
  void MoveSprite(int id, int px, int py);
  
  void DrawBackground();

};
