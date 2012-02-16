#include "TiltFlowDetailView.h"

namespace TotalsGame {

   DEFINE_POOL(TiltFlowDetailView);

    TiltFlowDetailView::TiltFlowDetailView(TotalsCube *c): InterstitialView(c)
    {/* TODO
      message = "";
      image = "neighbor_for_details";
      mImageOffset = 18; */
        mAmount = 0;
        mDescription = "";
    }
#if 0 //todo
    public void ShowDescription(string desc) {
      if (mDescription == desc) { return; }
      if (mDescription.Length == 0) {
        Jukebox.Sfx("PV_menu_tilt_stop");
      }
      mDescription = desc;
      Paint();
    }

    public void HideDescription() {
      if (mDescription.Length > 0) {
        Jukebox.Sfx("PV_menu_tilt_stop");
        mDescription = "";
      }
    }

    public override void Update (float dt) {
      if (mDescription.Length > 0) {
        if (mAmount < 1f) {
          mAmount = Mathf.Min(mAmount + dt / SlideHelper.kTransitionTime, 1f);
          Paint();
        }
      } else {
        if (mAmount > 0f) {
          mAmount = Mathf.Max(mAmount - dt / SlideHelper.kTransitionTime, 0f);
          Paint();
        }
      }
    }

    public override void Paint (Cube c) {
      if (mAmount == 0f) {
        base.Paint(c);
      } else {
        int bottom = Mathf.FloorToInt(Mathf.Lerp(32, 128-32, Mathf.Clamp01(1.1f * mAmount)));
        c.FillRect(new Color(75, 0, 85), 0, 8, 128, bottom); // background
        c.Image("vault_door", 0, 8-128);
        c.Image("vault_door", 0, bottom);
        if (mAmount == 1) {
          Library.PskFont.Paint(c, mDescription, new Int2(8, 24), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128-8-8, 128-32-32));
        }
      }


    }
#endif

}

