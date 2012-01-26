#pragma once
#include "Base.h"
#include "Content.h"

class Room;

class GameView {
public:

  // hack
  bool visited;

private:
  unsigned mRoomId;

  union {
    struct {
      unsigned start_frame;
      unsigned count;
    } idle;
    struct {
      unsigned start_frame;
    } room;
  } mScene;

public:  
  GameView();
  
  // getters
  Cube* GetCube() const;
  bool IsShowingRoom() const;
  bool InSpriteMode() const;
  Vec2 Location() const;
  Room* CurrentRoom() const;
  Cube::Side VirtualTiltDirection() const;
  GameView* VirtualNeighborAt(Cube::Side side) const;
  
  // methods
  void Init();
  void Update();
  
  bool ShowLocation(Vec2 loc);
  bool HideRoom();
  
  void ShowPlayer();
  void SetPlayerFrame(unsigned frame);
  void UpdatePlayer();
  void HidePlayer();
  
  void SetItemPosition(Vec2 p);
  void HideItem();

  void RefreshInventory();
  void DrawBackground();

private:
  void DrawInventorySprites();
  void HideInventorySprites();

  // misc hacky stuff
  void RandomizeBff();
};
