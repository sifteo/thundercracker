#pragma once
#include "Base.h"

class Enemy;
struct MapRoom;

class GameView {
public:
  int visited;

private:
  Vec2 mRoom;
  
public:  
  GameView();
  
  // getters
  Cube* GetCube() const;
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
  void SetPlayerFrame(unsigned frame);
  void UpdatePlayer();
  void HidePlayer();
  
  void ShowEnemy(Enemy* pEnemy);
  void UpdateEnemy(Enemy* pEnemy);
  void HideEnemy(Enemy* pEnemy);
  
  void ShowItem(int itemId);
  void SetItemPosition(Vec2 p);
  void HideItem();

  void DrawBackground();

};
