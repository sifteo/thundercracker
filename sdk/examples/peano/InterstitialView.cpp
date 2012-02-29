#include "InterstitialView.h"
#include "TotalsCube.h"
#include "Game.h"

namespace TotalsGame {

//DEFINE_POOL(InterstitialView);

InterstitialView::InterstitialView(TotalsCube *c) : View(c)
{
    message = "Starting Simple";
    image = NULL;
    mOffset = 0;
    mBackwards = false;
    mImageOffset = 0;
}

void InterstitialView::SetTransitionAmount(float u)
{
    // 0 <= offset <= 136+2*kPad (56, kPad, 48, kPad, 32)
    SetTransition(kMaxOffset * u);
}

void InterstitialView::TransitionSync(float duration, bool opening)
{
    for(float t=0; t<duration; t+=Game::GetInstance().dt) {
        SetTransitionAmount((opening? t : 1-t)/duration);
        Paint();
        System::paintSync();
        Game::GetInstance().UpdateDt();
    }
    SetTransitionAmount(opening? 1 : 0);
    Paint();
}

void InterstitialView::SetTransition(int offset)
{
    offset = CollapsesPauses(offset);
    offset = MIN(offset, 17);
    offset = MAX(offset, 0);
    if (mOffset != offset)
    {
        mBackwards = offset < mOffset;
        mOffset = offset;
        //TODO      Paint();
    }
}

int InterstitialView::CollapsesPauses(int off)
{
    if (off > 7)
    {
        if (off < 7 + kPad)
        {
            return 7;
        }
        off -= kPad;
        if (off > 7 + 6)
        {
            if (off < 7 + 6 + kPad)
            {
                return 7 + 6;
            }
            off -= kPad;
        }
    }
    return off;
}

void InterstitialView::Paint()
{
    PaintWithOffset(GetCube(), mOffset, mBackwards);
    if (mOffset >= 17)
    {
        //TODO        if (message.Length > 0) Library.PskFont.PaintCentered(c, message, 64, 8+16);

        if (image && GetCube()->backgroundLayer.isSpriteHidden(0))
        {
            GetCube()->backgroundLayer.setSpriteImage(0, *image, 0);
            GetCube()->backgroundLayer.moveSprite(0, Vec2(64-8*image->width/2, 88 - 8*image->height/2 + mImageOffset));
        }
    }
    else
    {
        if(!GetCube()->backgroundLayer.isSpriteHidden(0))
            GetCube()->backgroundLayer.hideSprite(0);
    }
}

void InterstitialView::PaintWithOffsetNorm(TotalsCube *c, float u, bool backwards)
{
    // this is basically a big ugly hack so that other classes can use this same
    // functionality without being exposed to the details, which grew organically,
    // rather than as design -- you know how it is :P
    PaintWithOffset(c, CollapsesPauses(kMaxOffset * u), backwards);
}

void InterstitialView::PaintWithOffset(TotalsCube *c, int off, bool backwards)
{
    if (off < 7) {
        // moving to the left
        c->ClipImage(&VaultDoor, Vec2(7-off, 7));
        c->ClipImage(&VaultDoor, Vec2(7-17-off,7));
        c->ClipImage(&VaultDoor, Vec2(7-off, 7-16));
        c->ClipImage(&VaultDoor, Vec2(7-16-off, 7-16));
    } else if (off < 7+6)
    {
        // moving up
        off -= 7;
        c->ClipImage(&VaultDoor, Vec2(0, 7-off));
        c->ClipImage(&VaultDoor, Vec2(0, 7-off-16));
    } else {
        // opening
        off -= (7 + 6);
        if (!backwards && off > 0)
        {
            c->FillArea(&Dark_Purple, Vec2(0, 1), Vec2(16, off));
        }
        c->ClipImage(&VaultDoor, Vec2(0, 1-16));
        c->ClipImage(&VaultDoor, Vec2(0, 1+off));
    }
}

}

