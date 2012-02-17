#pragma once

#include "sifteo.h"
#include "View.h"
#include "assets.gen.h"
#include "ObjectPool.h"

namespace TotalsGame {

    class TotalsCube;

class InterstitialView : public View
{
    DECLARE_POOL(InterstitialView, 2);

public:
    InterstitialView(TotalsCube *c);
    virtual ~InterstitialView() {};

    static const int kPad = 1;
    static const int kMaxOffset = 17 + kPad + kPad;

    void SetTransitionAmount(float u);

public:
    const char *message;

    const PinnedAssetImage *image;
private:
    int mOffset;
    bool mBackwards;


protected:
    int mImageOffset;

private:
    void SetTransition(int offset);

    static int CollapsesPauses(int off);
public:
    void Paint();

    static void PaintWithOffsetNorm(TotalsCube *c, float u, bool backwards);

    static void PaintWithOffset(TotalsCube *c, int off, bool backwards);
};
}

