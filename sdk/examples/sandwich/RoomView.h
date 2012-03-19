#pragma once
#include "Common.h"
#include "View.h"
#include "Content.h"
#include "DrawingHelpers.h"
#include "Sokoblock.h"

class Room;
#define ANIM_TILE_CAPACITY 4
#define ITEM_OFFSET 34

class RoomView : public View {
private:
  Sokoblock *mBlock;

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
  unsigned Id() const { return mRoomId; }
  Vec2 Location() const;
  Room* GetRoom() const;
  bool GatewayTouched() const;
  Sokoblock* Block() const { return mBlock; }

  // methods
  void Init(unsigned rid);
  void Restore();
  void Update(float dt);

  void HideOverlay(bool flag);
  
  void ShowPlayer();
  void ShowItem();
  void ShowBlock(Sokoblock* pBlock);

  void SetPlayerFrame(unsigned frame);
  void SetEquipPosition(Vec2 p);
  void SetItemPosition(Vec2 p);

  void UpdatePlayer();
  void DrawPlayerFalling(int height);
  void UpdateBlock();

  void HidePlayer();
  void HideItem();
  void HideEquip();
  void HideBlock();

  void DrawTrapdoorFrame(int delta);
  void DrawBackground();

private:
  void ComputeAnimatedTiles();

};
