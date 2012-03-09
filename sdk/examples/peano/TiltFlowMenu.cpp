#include "TiltFlowMenu.h"
#include "TiltFlowView.h"
#include "TiltFlowItem.h"
#include "Game.h"

namespace TotalsGame
{

TiltFlowItem *TiltFlowMenu::GetItem(int i)
{
    return items+i;
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

TiltFlowMenu::TiltFlowMenu(TiltFlowItem *_items, int _numItems, TiltFlowView * tfv, TiltFlowDetailView *_details)
{


    mSimTime = 0;
    mPickTime = 0;
    items = _items;
    numItems = _numItems;

    view = tfv;
    view->menu = this;
    details = _details;
    toggledItem = NULL;
}

float TiltFlowMenu::GetSimTime()
{
    return mSimTime;
}
TiltFlowItem *TiltFlowMenu::GetResultItem()
{
    return IsDone() ? view->GetItem() : NULL;
}
TiltFlowItem *TiltFlowMenu::GetToggledItem()
{
    return toggledItem;
}

int TiltFlowMenu::GetToggledItemIndex()
{
    for(int i = 0; i < numItems; i++)
    {
        if(toggledItem == items+i)
            return i;
    }
    return -1;
}

void TiltFlowMenu::SetToggledItem(TiltFlowItem *item)
{
    toggledItem = item;
}

void TiltFlowMenu::ClearToggledItem()
{
    SetToggledItem(NULL);
}
bool TiltFlowMenu::IsPicked()
{
    return mPickTime > 0;
}
bool TiltFlowMenu::IsDone()
{
    return IsPicked() && mSimTime - mPickTime > kPickDelay;
}

void TiltFlowMenu::Tick()
{
    mSimTime += Game::dt;
    view->Tick();
}

void TiltFlowMenu::Pick()
{
    if (!IsPicked()) {
        mPickTime = mSimTime;
    }
}

}

