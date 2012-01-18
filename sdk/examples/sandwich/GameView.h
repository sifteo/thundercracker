#pragma once
#include "Base.h"
#include "Content.h"

class Room;

class GameView {
public:
  int visited;

private:
  Vec2 mRoom;

  int mIdleHoverIndex;
  // h4cky scene-specific stuff
  union {
    struct {
      uint16_t count;
      uint16_t time;
    } idle;
    struct {
      uint32_t torchTime;
    } dungeon;
    struct {
      uint8_t hasBff;
      uint8_t bffDir;
      uint8_t bffX;
      uint8_t bffY;
    } forest;
  } mScene;

public:  
  GameView();
  
  // getters
  Cube* GetCube() const;
  bool IsShowingRoom() const;
  bool InSpriteMode() const;
  Vec2 Location() const { return mRoom; }
  Room* CurrentRoom() const;
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
