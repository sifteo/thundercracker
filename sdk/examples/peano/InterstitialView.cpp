#include "InterstitialView.h"
#include "TotalsCube.h"

namespace TotalsGame {



    InterstitialView::InterstitialView(TotalsCube *c) : View(c)
    {
        message = "Starting Simple";
        image = &Hint_5;
        mOffset = 0;
        mBackwards = false;
        mImageOffset = 0;
    }


    void InterstitialView::SetTransitionAmount(float u)
    {
        // 0 <= offset <= 136+2*kPad (56, kPad, 48, kPad, 32)
        SetTransition(kMaxOffset * u);
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

            if (image)
            {
                GetCube()->Image(image, Vec2(8-image->width/2, 11 - image->height/2 + mImageOffset));
            }
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
            c->Image(&VaultDoor, Vec2(7-off, 7), Vec2(0,0), Vec2(16-7+off, 9));
            c->Image(&VaultDoor, Vec2(0,7), Vec2(16-7+off, 7), Vec2(7-off, 9));
            c->Image(&VaultDoor, Vec2(7-off, 0), Vec2(off-7 ,0), Vec2(16-7+off, 7));
            c->Image(&VaultDoor, Vec2(7-off, 0), Vec2(9+off, 9), Vec2(7-off, 7));
        } else if (off < 7+6)
        {
            // moving up
            off -= 7;
            c->Image(&VaultDoor, Vec2(0, 7-off), Vec2(0,0), Vec2(16, 16-7-off));
            c->Image(&VaultDoor, Vec2(0, 0), Vec2(0, 9+off), Vec2(16, 7-off));
        } else {
            // opening
            off -= (7 + 6);
            c->Image(&VaultDoor, Vec2(0, 0), Vec2(0,15), Vec2(16,1));
            if (!backwards && off > 0)
            {
//TODO                c.FillRect(new Color(75, 0, 85), 0, 8, 128, off);
            }
            c->Image(&VaultDoor, Vec2(0, 1+off), Vec2(0,0), Vec2(16,16-1-off));
        }
    }

}

