#include "IdleView.h"
#include "Game.h"

#define mCanvas         (Parent()->Canvas())  

const static uint8_t sHopTable[] = { 0, 0, 0, 1, 3, 4, 6, 6, 7, 7, 8, 7, 7, 6, 6, 4, 3, 1, };
#define HOP_COUNT 18
#define HOP_PHASE 9

void IdleView::Init() {
  mStartFrame = gGame.AnimFrame();
  Parent()->HideSprites();
  mCanvas.bg0.image(vec(0,0), Blank);
  mCanvas.bg1.eraseMask();
}

void IdleView::Restore() {
  Init();
}
