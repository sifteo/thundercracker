#include "InterstitialView.h"
#include "TotalsCube.h"
#include "Game.h"
#include "AudioPlayer.h"
#include "Skins.h"

namespace TotalsGame {

//DEFINE_POOL(InterstitialView);

InterstitialView::InterstitialView()
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
    if(opening)
    {
        AudioPlayer::PlayShutterOpen();
    }
    else
    {
        AudioPlayer::PlayShutterClose();
    }

    if (GetCube()->IsTextOverlayEnabled())
    {
        GetCube()->DisableTextOverlay();
    }

    for(float t=0; t<duration; t+=Game::dt) {
        SetTransitionAmount((opening? t : duration-t)/duration);
        System::paint();
        //System::finish();
        Game::UpdateDt();
    }
    SetTransitionAmount(opening? 1 : 0);
    System::paint();

    if (mOffset >= 17 && message[0] && !GetCube()->IsTextOverlayEnabled())
    {
        int fg[] = {75, 0, 85};
        int bg[] = {255 ,255, 255};
        GetCube()->EnableTextOverlay(message, 16, 20, fg, bg);
    }
}

void InterstitialView::SetTransition(int offset)
{
    offset = CollapsesPauses(offset);
    offset = MIN(offset, 17);
    offset = MAX(offset, 0);

    mBackwards = offset < mOffset;
    mOffset = offset;
    InterstitialView::Paint();

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
        if (image && message[0] && GetCube()->vid.sprites[0].isHidden())
        {
            GetCube()->vid.sprites[0].setImage(*image, 0);
            GetCube()->vid.sprites[0].move(vec(64-8*image->tileWidth()/2, 88 - 8*image->tileHeight()/2 + mImageOffset));
        }
    }
    else
    {
        if(!GetCube()->vid.sprites[0].isHidden())
            GetCube()->vid.sprites[0].hide();
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
    const Skins::Skin &skin = Skins::GetSkin();

    if (off < 7) {
        // moving to the left
        c->ClipImage(&skin.vault_door, vec(7-off, 7));
        c->ClipImage(&skin.vault_door, vec(7-16-off,7));
        c->ClipImage(&skin.vault_door, vec(7-off, 7-16));
        c->ClipImage(&skin.vault_door, vec(7-16-off, 7-16));
    } else if (off < 7+6)
    {
        // moving up
        off -= 7;
        c->ClipImage(&skin.vault_door, vec(0, 7-off));
        c->ClipImage(&skin.vault_door, vec(0, 7-off-16));
    } else {
        // opening
        off -= (7 + 6);
        if (!backwards && off > 0)
        {
            c->FillArea(&Dark_Purple, vec(0, 1), vec(16, off));
        }
        c->ClipImage(&skin.vault_door, vec(0, 1-16));
        c->ClipImage(&skin.vault_door, vec(0, 1+off));
    }
}

}

