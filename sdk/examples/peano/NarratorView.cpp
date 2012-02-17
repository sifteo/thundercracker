#include "NarratorView.h"
#include "string.h"
#include "assets.gen.h"
#include "TotalsCube.h"

namespace TotalsGame {

DEFINE_POOL(NarratorView);

const AssetImage *NarratorView::emotes[] =
{
    &Narrator_Coin,
    &Narrator_Diamond,
    &Narrator_Emerald,
    &Narrator_GetReady,
    NULL,
    NULL,
    &Narrator_Ruby,
    NULL,
    NULL,
    NULL,
    &Narrator_Base,
};

NarratorView::NarratorView(TotalsCube *c):View(c)
{
    mOffset = 0;
    mString="";
    mEmote=EmoteNone;
}

void NarratorView::SetMessage(const char *msg, Emote emote) {
    if (!strcmp(mString,msg) && mEmote == emote) { return; }
    mString = msg;
    mEmote = emote;
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
            AudioPlayer::PlaySfx(emote == EmoteWave ? sfx_Tutorial_Stinger_01 : sfx_Dialogue_Balloon);
            PaintText();
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
            //          Cube.DrawVaultDoorsOpenStep1(mOffset, "narrator_base");
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

    c->Image(&Narrator_Base, Vec2(0,0));
    if (mEmote != EmoteNone) {
        c->Image(emotes[mEmote], Vec2(2,8));//TODO approximation here 16, 65);
    }
    if (mOffset > 0) {
        c->DrawVaultDoorsOpenStep1(mOffset, &Narrator_Base);
    }


    if(mString[0]) {
        PaintText();
    }
}

void NarratorView::PaintText() {
    /* TODO
      const int pad = 8;
      Cube.Image("narrator_balloon", 6, 6, 0, 0, 116, 65);
      Library.Verdana.Paint(Cube, mString, new Int2(6+pad,6), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(116-pad-pad, 45));
*/
}



}

