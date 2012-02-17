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

    void ShowDescription(const char * desc);

    void HideDescription();

    void Update (float dt);

    void Paint();



  };
}

