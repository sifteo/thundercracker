#pragma once
#include "Base.h"
#include "Content.h"

class Room;
#define ANIM_TILE_CAPACITY 4

class GameView {
private:
  unsigned mRoomId;

  struct AnimTileView {
    uint8_t lid;
    uint8_t frameCount;
  };

  union {
    struct {
      unsigned startFrame;
      unsigned count;
    } idle;
    struct {
      unsigned startFrame;
      unsigned animTileCount;
      AnimTileView animTiles[ANIM_TILE_CAPACITY];
    } room;
  } mScene;

  struct {
    uint8_t hideOverlay : 1;
    uint8_t unused : 7;
  } flags;

public:  
  // getters
  Cube::ID GetCubeID() const;
  Cube* GetCube() const;
  bool IsShowingRoom() const;
  bool InSpriteMode() const;
  Vec2 Location() const;
  Room* GetRoom() const;
  Cube::Side VirtualTiltDirection() const;
  GameView* VirtualNeighborAt(Cube::Side side) const;
  
  // methods
  void Init();
  void Update();
  
  bool ShowLocation(Vec2 loc);
  void HideOverlay(bool flag);
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

  void ComputeAnimatedTiles();

  // misc hacky stuff
  void RandomizeBff();
};
