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

const AssetImage *NarratorView::emotes[] =
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

NarratorView::NarratorView(TotalsCube *c):View(c)
{
    mOffset = 0;
    mString="";
    mEmote=EmoteNone;
}

int strcmp(const char *a, const char *b);

void NarratorView::SetMessage(const char *msg, Emote emote) {
    if (!strcmp(mString,msg) && mEmote == emote) { return; }
    mString = msg;
    mEmote = emote;

    if(mString[0])
        GetCube()->EnableTextOverlay(msg, 8, 40, 255,255,255, 0,0,0);
    else
        GetCube()->DisableTextOverlay();

    /* TODO
  is it really necessary to draw the images here?
  theyll get drawnagain in Paint()
  */

    if (OkayToPaint()) {
        if (mEmote != EmoteNone) {
            //            GetCube().Image(emotes[mEmote], Vec2(16, 65, 0, 0, 94, 43);
        } else {
            //          Cube.Image("narrator_base", 16, 65, 16, 65, 94, 43);
        }
        if (mString[0]) {
            PLAY_SFX(emote == EmoteWave ? sfx_Tutorial_Stinger_01 : sfx_Dialogue_Balloon);
//            PaintText();
        } else {
            //          Cube.Image("narrator_base", 6, 6, 6, 6, 116, 65);
        }
        //        Cube.Paint();
    }
}

void NarratorView::SetEmote(Emote emote) {
    /* TODO see note in SetMessage */
    if (mEmote != emote) {
        mEmote = emote;
        if (OkayToPaint()) {
            if (mEmote!=EmoteNone) {
                //            Cube.Image("narrator_" + mEmote, 16, 65, 0, 0, 94, 43);
            } else {
                //            Cube.Image("narrator_base", 16, 65, 16, 65, 94, 43);
            }
            //          Cube.Paint();
        }
    }
}

void NarratorView::SetTransitionAmount(float u) {
    if (u < 0) {
        mOffset = -1;
        //        Paint();
    } else {
        mOffset = (16-7)* u;
        if (OkayToPaint()) {
                      GetCube()->DrawVaultDoorsOpenStep1(mOffset, &Narrator_Base);
            //          Cube.Paint();
        }
    }

}

void NarratorView::Paint() {
    TotalsCube *c = GetCube();

    if (mOffset == 0) {
        c->DrawVaultDoorsClosed();
        return;
    }

    c->Image(emotes[mEmote], Vec2(0,16-emotes[mEmote]->height));

    if (mOffset > 0) {
        c->DrawVaultDoorsOpenStep1(mOffset, &Narrator_Base);
    }


    if(mString[0]) {
        PaintText();
    }
}

void NarratorView::PaintText()
{
      const int pad = 8;
      //GetCube()->Image(&Narrator_Balloon, Vec2(0,0));
      GetCube()->foregroundLayer.DrawAsset(Vec2(0,0), Narrator_Balloon);
  //    Library.Verdana.Paint(Cube, mString, new Int2(6+pad,6), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(116-pad-pad, 45));

      //text wil actually be drawn after this frame completes
}



}

