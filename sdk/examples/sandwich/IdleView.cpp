#include "IdleView.h"

void IdleView::Init() {
  mStartFrame = pGame->AnimFrame();
  DrawInventorySprites();
  DrawBackground();
  return result;
}

void IdleView::Update() {
  VidMode_BG0_SPR_BG1 mode(Parent()->GetCube()->vbuf);
  // compute position of each inventory item based on phase and current time
  const unsigned t = pGame->AnimFrame() - mScene.idle.startFrame;
  if (mScene.idle.count > 0 && t <= HOP_PHASE * (mScene.idle.count-1) + HOP_COUNT) {
    int count = 0;
    const int pad = 24;
    const int innerPad = (128-pad-pad)/3;
    for(unsigned i=0; i<mScene.idle.count; ++i) {
      int localTime = t - HOP_PHASE * i;
      if (localTime < HOP_COUNT) {
        mode.moveSprite(i, pad + innerPad * i - 8, 108  - sHopTable[localTime]);
      } else {
        mode.moveSprite(i, pad + innerPad * i - 8, 108);
      }
    }
  }
}

void IdleView::OnInventoryChanged() {
	mStartFrame = pGame->AnimFrame();
  DrawInventorySprites();
}

void IdleView::DrawInventorySprites() {
  VidMode_BG0_SPR_BG1 mode(Parent()->GetCube()->vbuf);
  mScene.idle.count = 0;
  const int pad = 24;
  const int innerPad = (128-pad-pad)/3;
  const int firstSandwichId = 2;
  const int sandwichTypeCount = 4;
  for(int itemId=firstSandwichId; itemId<firstSandwichId+sandwichTypeCount; ++itemId) {
    if (pGame->GetPlayer()->HasItem(itemId)) {
      mode.resizeSprite(mScene.idle.count, 16, 16);
      mode.moveSprite(mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      mode.setSpriteImage(mScene.idle.count, Items.index + (itemId-1) * Items.width * Items.height);
      mScene.idle.count++;
    }
  }
  if (pGame->GetPlayer()->HaveBasicKey()) {
      mode.resizeSprite(mScene.idle.count, 16, 16);
      mode.moveSprite(mScene.idle.count, pad + innerPad * mScene.idle.count - 8, 108);
      mode.setSpriteImage(mScene.idle.count, Items.index + (ITEM_BASIC_KEY-1) * Items.width * Items.height);
      mScene.idle.count++;
  }
  for(int i=mScene.idle.count; i<4; ++i) {
    mode.hideSprite(i);
  }
}

void IdleView::DrawBackground() {
  VidMode_BG0 mode(Parent()->GetCube()->vbuf);
  mode.BG0_drawAsset(Vec2(0,0), *(pGame->GetMap()->Data()->blankImage));
  BG1Helper(*Parent()->GetCube()).Flush();
}

