#pragma once
#include "Common.h"
#include "View.h"
#include "Content.h"
#include "DrawingHelpers.h"
#include "Sokoblock.h"

class Room;
#define ANIM_TILE_CAPACITY 4
#define ITEM_OFFSET 34

#define WOBBLE_NOD          0
#define WOBBLE_SHAKE        1
#define WOBBLE_SLIDE_TOP    2
#define WOBBLE_SLIDE_LLEFT  3
#define WOBBLE_SLIDE_BOTTOM 4
#define WOBBLE_SLIDE_RIGHT  5
#define WOBBLE_UNUSED_0     6
#define WOBBLE_UNUSED_1     7

class RoomView : public View {
private:
  Sokoblock *mBlock;
  float mWobbles;
  uint8_t mRoomId;
  uint8_t mStartFrame;
  struct {
    uint8_t hideOverlay : 1;
    uint8_t locked : 1; // technically we don't need this flag -- gGame has a mask now
    uint8_t wobbleType : 3;
    uint8_t animTileCount : 3;
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
  Int2 Location() const;
  Room* GetRoom() const;
  bool GatewayTouched() const;
  Sokoblock* Block() const { return mBlock; }
  bool IsWobbly() const { return mWobbles > 0.0001f; }

  // methods
  void Init(unsigned rid);
  void Restore();
  void Update();

  bool Locked() const { return flags.locked; }
  void Lock();
  void Unlock();

  void HideOverlay(bool flag);
  
  void ShowPlayer();
  void ShowItem(const ItemData* item);
  void ShowBlock(Sokoblock* pBlock);
  void RefreshDoor();
  void RefreshDepot();

  void SetPlayerFrame(unsigned frame);
  void SetEquipPosition(Int2 p);
  void SetItemPosition(Int2 p);

  void UpdatePlayer();
  void DrawPlayerFalling(int height);
  void UpdateBlock();

  void HidePlayer();
  void HideItem();
  void HideEquip();
  void HideBlock();

  void StartNod();
  void StartShake();
  void StartSlide(Cube::Side side);

  void ShowFrame();

  void DrawTrapdoorFrame(int delta);
  void DrawBackground();

private:
  void ComputeAnimatedTiles();
  void HideOverlay();
};
