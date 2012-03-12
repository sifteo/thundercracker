#include "NarratorView.h"
#include "assets.gen.h"
#include "TotalsCube.h"

namespace TotalsGame {
/*
static int strcmp(const char *a, const char *b)
{
    int dif = 0;
    while(!dif && *a && *b)
        dif = *a++ - *b++;
    return dif;
}*/

const PinnedAssetImage *NarratorView::emotes[] =
{
    &Narrator_Coin,
    &Narrator_Diamond,
    &Narrator_Emerald,
    &Narrator_GetReady,
//    &Narrator_Mix01,
//    &Narrator_Mix02,
    &Narrator_Ruby,
    &Narrator_Sad,
    &Narrator_Wave,
    &Narrator_Yay,
    &Narrator_Base,
};

NarratorView::NarratorView()
{
    Reset();
}

void NarratorView::Reset()
{
    mOffset = 0;
    mString="";
    mEmote=EmoteNone;
}

int strcmp(const char *a, const char *b);

void NarratorView::SetMessage(const char *msg, Emote emote) {
    bool hadMessage = mString[0] != 0;
    bool emoteChanged = emote != mEmote;

    mString = msg;
    mEmote = emote;

    if(mString[0])
    {
        if(!hadMessage)
        {
            Paint();
            GetCube()->foregroundLayer.Flush();
        }
        else if(emoteChanged)
        {
            GetCube()->DisableTextOverlay();
            Paint();
        }
        GetCube()->EnableTextOverlay(msg, 8, 40, 255,255,255, 0,0,0);
    }
    else
    {
        GetCube()->DisableTextOverlay();
        Paint();
    }

    if (mString[0])
        PLAY_SFX(emote == EmoteWave ? sfx_Tutorial_Stinger_01 : sfx_Dialogue_Balloon);
}

void NarratorView::SetEmote(Emote emote) {
    if (mEmote != emote) {
        mEmote = emote;

        if (mEmote!=EmoteNone) {
            //            Cube.Image("narrator_" + mEmote, 16, 65, 0, 0, 94, 43);
        } else {
            //            Cube.Image("narrator_base", 16, 65, 16, 65, 94, 43);
        }
        //          Cube.Paint();
    }
}

void NarratorView::SetTransitionAmount(float u) {
    if (u < 0) {
        mOffset = -1;
        //        Paint();
    } else {
        mOffset = (16-7)* u;
        GetCube()->DrawVaultDoorsOpenStep1(mOffset, &Narrator_Base);
        //          Cube.Paint();
    }

}

void NarratorView::Paint() {
    TotalsCube *c = GetCube();

    if (mOffset == 0) {
        c->DrawVaultDoorsClosed();
        return;
    }

    //if(mEmote != EmoteNone)
    {
        //draw the top half since the emotes arent full screen.
        c->Image(&Narrator_Base, Vec2(0,0));//, Vec2(0,0), Vec2(16, 16-emotes[mEmote]->height));
    }
    
    if(mEmote ==EmoteMix01)
    {
        c->Image(&Narrator_Mix01, Vec2(0,Narrator_Mix01.height));
    }
    else if(mEmote ==EmoteMix02)
    {
                c->Image(&Narrator_Mix02, Vec2(0,16-Narrator_Mix02.height));
    }
    else if(mEmote != EmoteNone)
    {
    c->Image(emotes[mEmote], Vec2(0,16-emotes[mEmote]->height));
    }

    if (mOffset > 0) {
        c->DrawVaultDoorsOpenStep1(mOffset, &Narrator_Base);
    }


    if(mString[0]) {
        GetCube()->foregroundLayer.DrawAsset(Vec2(0,0), Narrator_Balloon);
    }
}



}

