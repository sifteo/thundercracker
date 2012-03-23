#pragma once
#include "InterstitialView.h"
#include "ObjectPool.h"
#include "AudioPlayer.h"
#include "TotalsCube.h"
#include "string.h"

namespace TotalsGame {

  class TiltFlowDetailView : public InterstitialView
  {

    float mAmount;
    const char *mDescription;
public:
    TiltFlowDetailView();
    virtual ~TiltFlowDetailView() {}

    void ShowDescription(const char * desc);

    void HideDescription();

    void Paint();

  };
}

