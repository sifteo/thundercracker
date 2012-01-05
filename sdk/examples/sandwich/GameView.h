#pragma once
#include "Base.h"

struct MapRoom;

class GameView {
public:
  int visited;

private:
  Vec2 mRoom;

  int mIdleHoverIndex;
  // h4cky torch stuff
  int mTorchTime;
  //

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
  void Update();

  bool ShowLocation(Vec2 room);
  bool HideRoom();
  
  void ShowPlayer();
  void SetPlayerFrame(unsigned frame);
  void UpdatePlayer();
  void HidePlayer();
  
  void ShowItem(int itemId);
  void SetItemPosition(Vec2 p);
  void HideItem();

  void RefreshInventory();

  void DrawBackground();

private:
  void DrawInventorySprites();
  void HideInventorySprites();
};
