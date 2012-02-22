#include "IdleView.h"
#include "Game.h"

const static uint8_t sHopTable[] = { 0, 0, 0, 1, 3, 4, 6, 6, 7, 7, 8, 7, 7, 6, 6, 4, 3, 1, };
#define HOP_COUNT 18
#define HOP_PHASE 9

void IdleView::Init() {
  mStartFrame = pGame->AnimFrame();
  //DrawInventorySprites();
  Parent()->HideSprites();
  Parent()->Graphics().BG0_drawAsset(Vec2(0,0), Blank);
  Parent()->Overlay().Flush();
}

void IdleView::Restore() {
  Init();
}

void IdleView::Update() {
  /*
  ViewMode mode = Parent()->Graphics();
  // compute position of each inventory item based on phase and current time
  const unsigned t = pGame->AnimFrame() - mStartFrame;
  if (mCount > 0 && t <= HOP_PHASE * (mCount-1) + HOP_COUNT) {
    int count = 0;
    const int pad = 24;
    const int innerPad = (128-pad-pad)/3;
    for(unsigned i=0; i<mCount; ++i) {
      int localTime = t - HOP_PHASE * i;
      if (localTime < HOP_COUNT) {
        mode.moveSprite(i, pad + innerPad * i - 8, 108  - sHopTable[localTime]);
      } else {
        mode.moveSprite(i, pad + innerPad * i - 8, 108);
      }
    }
  }
  */
}

/*
void IdleView::OnInventoryChanged() {
	mStartFrame = pGame->AnimFrame();
  DrawInventorySprites();
}

void IdleView::DrawInventorySprites() {
  ViewMode mode = Parent()->Graphics();
  mCount = 0;
  const int pad = 24;
  const int innerPad = (128-pad-pad)/3;
  const int firstSandwichId = 2;
  const int sandwichTypeCount = 4;
  for(int itemId=firstSandwichId; itemId<firstSandwichId+sandwichTypeCount; ++itemId) {
    if (pGame->GetPlayer()->HasItem(itemId)) {
      mode.resizeSprite(mCount, 16, 16);
      mode.moveSprite(mCount, pad + innerPad * mCount - 8, 108);
      mode.setSpriteImage(mCount, Items.index + (itemId-1) * Items.width * Items.height);
      mCount++;
    }
  }
  if (pGame->GetPlayer()->HaveBasicKey()) {
      mode.resizeSprite(mCount, 16, 16);
      mode.moveSprite(mCount, pad + innerPad * mCount - 8, 108);
      mode.setSpriteImage(mCount, Items.index + (ITEM_BASIC_KEY-1) * Items.width * Items.height);
      mCount++;
  }
  for(int i=mCount; i<4; ++i) {
    mode.hideSprite(i);
  }
}
*/

