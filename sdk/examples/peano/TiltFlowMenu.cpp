#include "TiltFlowMenu.h"
#include "TiltFlowView.h"
#include "TiltFlowItem.h"
#include "Game.h"

namespace TotalsGame
{

    TiltFlowItem *TiltFlowMenu::GetItem(int i)
    {
        return &items[i];
    }

    int TiltFlowMenu::GetNumItems()
    {
        return numItems;
    }

    TiltFlowView *TiltFlowMenu::GetView()
    {
        return view;
    }

    TiltFlowDetailView *TiltFlowMenu::GetDetails()
    {
        return details;
    }

    TiltFlowMenu::TiltFlowMenu(TiltFlowItem *_items, int _numItems)
    {
        mSimTime = 0;
        mPickTime = 0;
      items = _items;
      numItems = _numItems;
      view = new TiltFlowView(Game::GetCube(0), this);
//TODO      details = app.CubeSet.Find(cb => cb.userData is TiltFlowDetailView).GetView() as TiltFlowDetailView;
    }
/* TODO
    public void TiltFlowMenu::Dispose() {
      view.Cube = null;
    }
*/
    float TiltFlowMenu::GetSimTime()
    {
        return mSimTime;
    }
    TiltFlowItem *TiltFlowMenu::GetResultItem()
    {
        return IsDone() ? view->GetItem() : NULL;
    }
   //TODO what is this?! TiltFlowItem ToggledItem { get; set; }
    void TiltFlowMenu::ClearToggledItem()
    {
        //TODO wuuuut? SetToggledItem(NULL);
    }
    bool TiltFlowMenu::IsPicked()
    {
        return mPickTime > 0;
    }
    bool TiltFlowMenu::IsDone()
    {
        return IsPicked() && mSimTime - mPickTime > kPickDelay;
    }

    TiltFlowItem *TiltFlowMenu::GetToggledItem()
    {
        return toggledItem;
    }

    void TiltFlowMenu::SetToggledItem(TiltFlowItem *item)
    {
        toggledItem = item;
    }

    void TiltFlowMenu::Tick(float dt)
    {
      mSimTime += dt;
      view->Tick();
    }

    void TiltFlowMenu::Pick()
    {
        if (!IsPicked()) {
        // todo Log.Debug("Selected {0}", view.Item.name);
        mPickTime = mSimTime;
      }
    }

}

