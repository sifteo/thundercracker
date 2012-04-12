#include "TiltFlowDetailView.h"
#include "assets.gen.h"
#include "Game.h"
#include "Skins.h"

namespace TotalsGame {

//DEFINE_POOL(TiltFlowDetailView);

TiltFlowDetailView::TiltFlowDetailView()
{
    message = "";
    image = &Neighbor_For_Details;
    mImageOffset = 18;//pixel coord for sprite
    mAmount = 0;
    mDescription = "";
}


void TiltFlowDetailView::ShowDescription(const char * desc) {
    if (mDescription == desc) { return; }    //should be using same string pointer

    mDescription = desc;

    if (mDescription[0]) {
        PLAY_SFX(sfx_Menu_Tilt_Stop);
    }    

    if(GetCube()->IsTextOverlayEnabled())
    {
        //text box is already up, but we need to redraw it
        GetCube()->DisableTextOverlay();
    }

    while(mAmount < 1)
    {
        mAmount = MIN(mAmount + Game::dt / TotalsCube::kTransitionTime, 1);
        Paint();
        System::paintSync();
        Game::UpdateDt();
    }

    int fg[] = {75, 0, 85};
    int bg[] = {255 ,255, 255};
    GetCube()->EnableTextOverlay(mDescription, 24, 40, fg, bg);

}

void TiltFlowDetailView::HideDescription() {
    if (mDescription[0]) {
        if(mAmount > 0)
            PLAY_SFX(sfx_Menu_Tilt_Stop);

        mDescription = "";

        if(GetCube()->IsTextOverlayEnabled())
        {
            GetCube()->DisableTextOverlay();
        }

        while(mAmount > 0)
        {
            mAmount = MAX(mAmount - Game::dt / TotalsCube::kTransitionTime, 0);
            Paint();
            System::paintSync();
            Game::UpdateDt();
        }

        if(message[0])
        {
            int fg[] = {75, 0, 85};
            int bg[] = {255 ,255, 255};
            GetCube()->EnableTextOverlay(message, 16, 20, fg, bg);
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
        c->FillArea(&Dark_Purple, vec(0,1), vec(16,bottom-1));

        const Skins::Skin &skin = Skins::GetSkin();
        c->ClipImage(&skin.vault_door, vec(0,1-16));
        c->ClipImage(&skin.vault_door, vec(0, bottom));

        if(!GetCube()->backgroundLayer.isSpriteHidden(0))
        {
            GetCube()->backgroundLayer.hideSprite(0);   //enabled by interstitialview
        }
    }
}
}

