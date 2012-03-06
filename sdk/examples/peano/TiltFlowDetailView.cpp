#include "TiltFlowDetailView.h"
#include "assets.gen.h"
#include "Game.h"

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
        PLAY_SFX(sfx_Menu_Tilt_Stop);
    }
    mDescription = desc;
    //TODO Paint();
}

void TiltFlowDetailView::HideDescription() {
    if (mDescription[0]) {
        PLAY_SFX(sfx_Menu_Tilt_Stop);
        mDescription = "";        
        GetCube()->DisableTextOverlay();
    }
}

void TiltFlowDetailView::Update () {
    if (mDescription[0]) {
        while (mAmount < 1) {
            mAmount = MIN(mAmount + Game::dt / TotalsCube::kTransitionTime, 1);
            Paint();
            System::paintSync();
            Game::UpdateDt();
        }
    } else {
        while (mAmount > 0) {
            mAmount = MAX(mAmount - Game::dt / TotalsCube::kTransitionTime, 0);
            Paint();
            System::paintSync();
            Game::UpdateDt();
        }
    }


}

void TiltFlowDetailView::Paint() {
    TotalsCube *c = GetCube();
    if (mAmount == 0) {
        InterstitialView::Paint();
    } else {
#define INTERPOLATE(a,b,t)  ((a)*(1-(t))+(b)*(t))
        int bottom = (INTERPOLATE(4, 16-4, clamp(1.1f * mAmount,0.0f,1.0f)));
#undef INTERPOLATE
        c->FillArea(&Dark_Purple, Vec2(0,1), Vec2(16,bottom-1));
        c->ClipImage(&VaultDoor, Vec2(0,1-16));
        c->ClipImage(&VaultDoor, Vec2(0, bottom));

        if(!GetCube()->backgroundLayer.isSpriteHidden(0))
        {
            GetCube()->backgroundLayer.hideSprite(0);   //enabled by interstitialview
        }
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

