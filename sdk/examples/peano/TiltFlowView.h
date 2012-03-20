#pragma once

#include "View.h"
#include "TiltFlowMenu.h"
#include "TotalsCube.h"
#include "ObjectPool.h"

namespace TotalsGame {

  class TiltFlowView : public View
  {
      class EventHandler: public TotalsCube::EventHandler
      {
          TiltFlowView *owner;
      public:
          EventHandler(TiltFlowView* v) {owner = v;}
          void OnCubeTouch(TotalsCube *cube, bool touching) {owner->OnButton(cube, touching);}
      };
      EventHandler eventHandler;


    static const float kDriftVel = 1.5f;
    static const float kTiltVel = 3;
    static const float kMinAccel;
    static const float kMaxAccel;
    static const float kDeepTiltAccel = 2;
    static const float kGravity = 1.03f;
    static const float kMarqueeDelay = 4;
    static const AssetImage *kMarquee[2];

private:
    int mItem ;
    float mOffsetX ;
    float mAccel;
    float mRestTime;
    bool mDrawLabel;
    bool mDirty;
    int mMarquee;
    float mLastUpdate;
    int mLastTileX;
public:
    TiltFlowView();
    virtual ~TiltFlowView() {}

    int GetItemIndex();
    TiltFlowItem *GetItem();

    void Tick();

    TiltFlowMenu *menu;

    //-------------------------------------------------------------------------
    // VIEW METHODS
    //-------------------------------------------------------------------------

    void DidAttachToCube(TotalsCube *c);

    void WillDetachFromCube(TotalsCube *c);

private:
    void PaintFooter(TotalsCube *c);
public:
    void PaintInner(TotalsCube *c);
private:
    void PixelToTileImage(const AssetImage *image, Vec2 p, Vec2 o, Vec2 s);

    void DoPaintItem(TiltFlowItem *item, int x);

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
  };
}

