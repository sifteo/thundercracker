#pragma once
#include "Common.h"
#include "View.h"
#include "Content.h"
#include "DrawingHelpers.h"

class Room;
#define ANIM_TILE_CAPACITY 4
#define ITEM_OFFSET 34

class RoomView : public View {
private:
  uint8_t mRoomId;

  uint8_t mStartFrame;
  uint8_t mAnimTileCount;
  struct {
    uint8_t hideOverlay : 1;
  } flags;

  struct AnimTile {
    uint8_t lid;
    uint8_t frameCount;
  };
  AnimTile mAnimTiles[ANIM_TILE_CAPACITY];

  union {
    ButterflyFriend bff;
  } mAmbient;

public:  
  // getters
  Vec2 Location() const;
  Room* GetRoom() const;
  
  // methods
  void Init(unsigned rid);
  void Restore();
  void Update(float dt);

  void HideOverlay(bool flag);
  
  void ShowPlayer();
  void SetPlayerFrame(unsigned frame);
  void UpdatePlayer();
  void DrawPlayerFalling(int height);
  void HidePlayer();

  void ShowItem();
  void SetEquipPosition(Vec2 p);
  void SetItemPosition(Vec2 p);
  void HideItem();
  void HideEquip();

  void DrawTrapdoorFrame(int delta);

  void DrawBackground();

private:
  void ComputeAnimatedTiles();
};
