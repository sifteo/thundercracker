#include "TiltFlowDetailView.h"
#include "assets.gen.h"

namespace TotalsGame {

//DEFINE_POOL(TiltFlowDetailView);

TiltFlowDetailView::TiltFlowDetailView(TotalsCube *c): InterstitialView(c)
{
    message = "";
    image = &Neighbor_For_Details;
    mImageOffset = 18;//pixel coord for sprite
    mAmount = 0;
    mDescription = "";
}
void TiltFlowDetailView::ShowDescription(const char * desc) {
    if (mDescription == desc) { return; }    //should be using same string pointer
    if (mDescription[0]) {
        AudioPlayer::PlaySfx(sfx_Menu_Tilt_Stop);
    }
    mDescription = desc;
    //TODO Paint();
}

void TiltFlowDetailView::HideDescription() {
    if (mDescription[0]) {
        AudioPlayer::PlaySfx(sfx_Menu_Tilt_Stop);
        mDescription = "";
        GetCube()->DisableTextOverlay();
    }
}

void TiltFlowDetailView::Update (float dt) {
    if (mDescription[0]) {
        if (mAmount < 1) {
            mAmount = MIN(mAmount + dt / TotalsCube::kTransitionTime, 1);
            //TODO    Paint();
        }
    } else {
        if (mAmount > 0) {
            mAmount = MAX(mAmount - dt / TotalsCube::kTransitionTime, 0);
            //TODO    Paint();
        }
    }


}

void TiltFlowDetailView::Paint() {
    TotalsCube *c = GetCube();
    if (mAmount == 0) {
        InterstitialView::Paint();
    } else {
#define INTERPOLATE(a,b,t)  ((a)*(1-(t))+(b)*(t))
        int bottom = /*Mathf.FloorToInt todo*/(INTERPOLATE(4, 16-4, clamp(1.1f * mAmount,0.0f,1.0f)));
#undef INTERPOLATE
        c->Image(&Dark_Purple, Vec2(0,1), Vec2(0,0), Vec2(16,bottom));
        c->ClipImage(&VaultDoor, Vec2(0,1-16));
        c->ClipImage(&VaultDoor, Vec2(0, bottom));

    }
/*
    if (mAmount == 1) {
        GetCube()->EnableTextOverlay(mDescription, 24, 40, 75,0,85, 255,255,255);
        //TODO      Library.PskFont.Paint(c, mDescription, new Int2(8, 24), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128-8-8, 128-32-32));
    }
    else
    {
        GetCube()->DisableTextOverlay();
    }
   */
}
}

