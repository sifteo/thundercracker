#pragma once
#include "InterstitialView.h"
#include "ObjectPool.h"
#include "AudioPlayer.h"
#include "TotalsCube.h"
#include "string.h"

namespace TotalsGame {

  class TiltFlowDetailView : public InterstitialView
  {

      DECLARE_POOL(TiltFlowDetailView, 2);

    float mAmount;
    const char *mDescription;
public:
    TiltFlowDetailView(TotalsCube *c);

    void ShowDescription(const char * desc) {
      if (!strcmp(mDescription,desc)) { return; }
      if (mDescription[0]) {
        AudioPlayer::PlaySfx(sfx_Menu_Tilt_Stop);
      }
      mDescription = desc;
      //TODO Paint();
    }

    void HideDescription() {
        if (mDescription[0]) {
        AudioPlayer::PlaySfx(sfx_Menu_Tilt_Stop);
        mDescription = "";
      }
    }

    void Update (float dt) {
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

    void Paint() {
        TotalsCube *c = GetCube();
      if (mAmount == 0) {
        InterstitialView::Paint();
      } else {
        #define INTERPOLATE(a,b,t)  ((a)*(1-(t))+(b)*(t))
        int bottom = /*Mathf.FloorToInt todo*/(INTERPOLATE(32, 128-32, clamp(1.1f * mAmount,0.0f,1.0f)));
        #undef INTERPOLATE
        //TODO c.FillRect(new Color(75, 0, 85), 0, 8, 128, bottom); // background
        c->ClipImage(&VaultDoor, Vec2(0,8-128));
        c->ClipImage(&VaultDoor, Vec2(0, bottom));
        if (mAmount == 1) {
    //TODO      Library.PskFont.Paint(c, mDescription, new Int2(8, 24), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128-8-8, 128-32-32));
        }
      }


    }

  };
}

