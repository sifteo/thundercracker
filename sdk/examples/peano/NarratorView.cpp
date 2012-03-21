#include "NarratorView.h"
#include "assets.gen.h"
#include "TotalsCube.h"

namespace TotalsGame {

const Sifteo::AssetImage *NarratorView::emotes[] =
{
    &Narrator_Coin,
    &Narrator_Diamond,
    &Narrator_Emerald,
    &Narrator_GetReady,
    &Narrator_Mix01,
    &Narrator_Mix02,
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
        GetCube()->Image(Narrator_Base);
        GetCube()->DrawVaultDoorsOpenStep1(mOffset);
        //          Cube.Paint();
    }

}

void NarratorView::Paint() {
    TotalsCube *c = GetCube();

    if (mOffset == 0) {
        c->DrawVaultDoorsClosed();
        return;
    }

    c->Image(Narrator_Base);
    if(mEmote != EmoteNone)
    {
        c->Image(*emotes[mEmote], Vec2<int>(0,16-emotes[mEmote]->height));
    }

    if (mOffset > 0) {
        c->DrawVaultDoorsOpenStep1(mOffset);
    }


    if(mString[0]) {
        GetCube()->foregroundLayer.DrawAsset(Vec2(0,0), Narrator_Balloon);
    }
}



}

