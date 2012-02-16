#pragma once

namespace TotalsGame
{

  class TiltFlowMenu /* TODO : IDisposable*/ {

    static const float kPickDelay = 0.25f;
    static const float kRestDelay = 0.1f;

    TiltFlowItem *items;
    int numItems;
  public:
    TiltFlowItem *GetItem(int i) {return items[i];}
  private:
    float mSimTime;
    float mPickTime;
    TiltFlowView *view;
  public:
    TileFlowView *GetView() {return view;}
  private:
    TiltFlowDetailView *details;
  public:
    TiltFlowDetailView *GetDetails() {return details;}

    TiltFlowMenu(TiltFlowItem *_items, int _numItems) {
        mSimTime = 0;
        mPickTime = 0;
      items = _items;
      numItems = _numItems;
      view = new TiltFlowView(this);
      view->SetCube(Game::GetCube(0));
      details = app.CubeSet.Find(cb => cb.userData is TiltFlowDetailView).GetView() as TiltFlowDetailView;
    }
/* TODO
    public void Dispose() {
      view.Cube = null;
    }
*/
    float GetSimTime() { return mSimTime; }
    TiltFlowItem *GetResultItem() { return IsDone() ? view->Item : NULL; }
   //TODO what is this?! TiltFlowItem ToggledItem { get; set; }
    void ClearToggledItem() { SetToggledItem(NULL); }
    bool IsPicked() { return mPickTime > 0; }
    bool IsDone() { return IsPicked() && mSimTime - mPickTime > kPickDelay; }

    void Tick(float dt) {
      mSimTime += dt;
      view->Tick();
    }
private:
    void Pick() {
        if (!IsPicked()) {
        // todo Log.Debug("Selected {0}", view.Item.name);
        mPickTime = mSimTime;
      }
    }

  };
}

