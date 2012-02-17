#pragma once

#include "View.h"
#include "TiltFlowMenu.h"
#include "TotalsCube.h"
#include "ObjectPool.h"

namespace TotalsGame {

  class TiltFlowView : public View
  {
      DECLARE_POOL(TiltFlowView, 2);

      class EventHandler: public TotalsCube::EventHandler
      {
          TiltFlowView *owner;
      public:
          EventHandler(TiltFlowView* v) {owner = v;}
      };
      EventHandler eventHandler;


    static const float kDriftVel = 1.5f;
    static const float kTiltVel = 3;
    static const float kMinAccel = 1;
    static const float kMaxAccel = 2.5f;
    static const float kDeepTiltAccel = 2;
    static const float kGravity = 1.03f;
    static const float kMarqueeDelay = 4;
    static const PinnedAssetImage *kMarquee[2];

private:
    TiltFlowMenu *menu;
public:
    TiltFlowMenu *GetTiltFlowMenu();
private:
    int mItem ;
    float mOffsetX ;
    float mAccel;
    float mRestTime;
    bool mDrawLabel;
    bool mDirty;
    int mMarquee;
    float mLastUpdate;
public:
    TiltFlowView(TotalsCube *c, TiltFlowMenu *_menu);

    int GetItemIndex();
    TiltFlowItem *GetItem();

    void Tick();

    //-------------------------------------------------------------------------
    // VIEW METHODS
    //-------------------------------------------------------------------------

    void DidAttachToCube(TotalsCube *c);

    void WillDetachFromCube(TotalsCube *c);

    void Paint();
private:
    void PaintFooter(TotalsCube *c);
public:
    void PaintInner(TotalsCube *c);
private:
    void PixelToTileImage(const AssetImage *image, const Vec2 &p, const Vec2 &o, const Vec2 &s);

    void DoPaintItem(TiltFlowItem *item, int x, int w);

    //-------------------------------------------------------------------------
    // EVENTS
    //-------------------------------------------------------------------------

    void OnButton(TotalsCube *c, bool pressed);


    //-------------------------------------------------------------------------
    // HELPERS
    //-------------------------------------------------------------------------

    void UpdateMenu();

    void CoastToStop();

    void StopScrolling();

    void ClipIt(int ox, int *x, int *w);
  };
}

