#pragma once

#include "sifteo.h"
#include <stddef.h>

namespace TotalsGame
{
class TiltFlowView;
class TiltFlowItem;
class TiltFlowDetailView;

  class TiltFlowMenu {
public:
    static const float kPickDelay = 0.25f;
    static const float kRestDelay = 0.1f;

    TiltFlowMenu(TiltFlowItem **_items, int _numItems, TiltFlowDetailView *_details);

    TiltFlowItem *GetItem(int i);
    int GetNumItems();
    TiltFlowView *GetView();
    TiltFlowDetailView *GetDetails();
    float GetSimTime();
    TiltFlowItem *GetResultItem();
    TiltFlowItem *GetToggledItem();
    int GetToggledItemIndex();
    void SetToggledItem(TiltFlowItem *item);
    void ClearToggledItem();
    bool IsPicked();
    bool IsDone();
    void Tick();
    void Pick();

private:
    TiltFlowItem **items;
    int numItems;
    float mSimTime;
    float mPickTime;
    TiltFlowView *view;
    TiltFlowDetailView *details;
    TiltFlowItem *toggledItem;

  public:
    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}
  };
}

