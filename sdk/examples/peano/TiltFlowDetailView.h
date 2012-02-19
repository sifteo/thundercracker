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
    TiltFlowDetailView(TotalsCube *c);
    virtual ~TiltFlowDetailView() {}

    void ShowDescription(const char * desc);

    void HideDescription();

    void Update (float dt);

    void Paint();

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}

  };
}

