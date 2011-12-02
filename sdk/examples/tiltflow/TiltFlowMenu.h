  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

#ifndef _TILTFLOWMENU_H
#define _TILTFLOWMENU_H


using namespace Sifteo;

class TiltFlowMenu
{
public:
    static const int MAX_ITEMS = 40;
    static const int MAX_CUBES = 32;

    const float UPDATE_DELAY;
    const float PICK_DELAY;
    const float REST_DELAY;

    typedef enum
    { CHOOSING, PICKED }
    Status;

    TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes);

    void Tick(float dt);

private:
    void Pick(TiltFlowView &view);
    void CheckMenuNeighbors();
    void ReassignMenu();

    //just have an array that can hold the max items
    TiltFlowItem *mItems[ MAX_ITEMS ];
    Status mStatus;
    bool mDone;
    float mSimTime;
    float mUpdateTime;
    float mPickTime;
    int mNumCubes;
    int mNumItems;
    TiltFlowView *mKeyView;
    TiltFlowView mViews[ MAX_CUBES ];
    bool mNeighborDirty;
};

/*
    public Action<TiltFlowView> PaintNone { internal get; set; }
    public Action<TiltFlowView> PaintBackground { internal get; set; }
    public Action<TiltFlowItemArgs> PaintItem { internal get; set; }

    public TiltFlowItem ResultItem { get { return mDone ? mKeyView.Item : null; } }

    internal void DirtyNeighbors() {
      mNeighborDirty = true;
    }
*/


    //TODO CAN'T DO FLIP YET
    /*internal void CheckFlip() {
      foreach(Cube c in app.CubeSet) {
        if (c.IsUpright) {
          mDone = false;
          return;
        }
      }
      mDone = true;
    }*/


  //---------------------------------------------------------------------------
  // TILT FLOW ITEM
  //---------------------------------------------------------------------------

  struct TiltFlowItemArgs {
    TiltFlowView view;
    TiltFlowItem item;
    AABB bounds;
  };

  class TiltFlowItem {
public:
    //public readonly Color color;
      char *mName;
      //char mDescription[32];
      Sifteo::AssetImage &mImage;
      //Vec2 mSrcPos;
    //public readonly Int2 sourcePosition;

    TiltFlowItem(Sifteo::AssetImage &image, char *pName/*, string description, Int2 sourcePosition=new Int2()*/);
};

  //---------------------------------------------------------------------------
  // TILT FLOW VIEW
  //---------------------------------------------------------------------------

  class TiltFlowView {
public:
    static const float DRIFTVEL;
    static const float TILTVEL;
    static const float MINACCEL;
    static const float MAXACCEL;
    static const float DEEPTILTACCEL;
    static const float GRAVITY;
    typedef enum
    { STATUS_NONE, STATUS_MENU, STATUS_ITEM, STATUS_INFO }
    Status;

    /*public Status CurrentStatus { get { return mStatus; } }
    public int ItemIndex { get { return mItem; } }
    public TiltFlowItem Item { get { return menu[mItem]; } }*/

    //public readonly TiltFlowMenu menu;

    void Paint(Cube c);
private:
    Status mStatus;
    int mItem;
    float mOffsetX;
    float mAccel;
    float mRestTime;
    bool mDrawLabel;
    bool mDirty;
    Side mLastNeighborRemoveSide;

    // for PositionOf and FindGroups
    bool mVisited;
    Int2 mGridPosition;

    TiltFlowView();

    void CheckForRepaint();

    void Tick();


    void PaintNone(Cube c);
    void PaintInfo(Cube c);
    void PaintMenu(Cube c);

    void DoPaintItem(TiltFlowItem item, int x, int w);
    void PaintItem(Cube c);

    void OnButton(Cube c, bool pressed);

    //TODO FLIP
    //void OnFlip(Cube c, bool newOrientationIsUp);

    void OnNeighborAdd(Cube c, Side s, Cube nc, Side ns);
    void OnNeighborRemove(Cube c, Side s, Cube nc, Side ns);

    internal void RelateToMenu() {
      var oldStatus = mStatus;
      var oldItem = mItem;
      Int2 pos;
      if (menu.KeyView.PositionOf(Cube, out pos)) {
        if (pos.y == 0) {
          mItem = menu.KeyView.mItem + pos.x;
          if (mItem >= 0 && mItem < menu.ItemCount) {
            if (mStatus != STATUS_ITEM) {
              mStatus = STATUS_ITEM;
              mAccel = MAXACCEL;
              mDrawLabel = false;
              if (pos.x > 0) {
                mOffsetX = -40; // magic
              } else {
                mOffsetX = 40; // magic
              }
            }
          } else {
            mStatus = Status.None;
          }
        } else if (pos.y == -1 || pos.y == 1) {
          mItem = menu.KeyView.mItem + pos.x;
          if (mItem >= 0 && mItem < menu.ItemCount) {
            mStatus = STATUS_INFO;
            mOffsetX = 0;
          } else {
            mStatus = Status.None;
          }
        } else {
          mStatus = Status.None;
        }
      } else {
        mStatus = Status.None;
      }
      if (mItem != oldItem || mStatus != oldStatus) {
        Hacky.Sfx("ui_select_02");
      }
      mDirty |= mStatus != oldStatus;
      mDirty |= mItem != oldItem;
    }

    void UpdateMenu() {
      if (
        Cube.Tilt[0] == 1 ||
        (Cube.Tilt[0] < 1 && mItem == menu.ItemCount-1 && mOffsetX >= 0) ||
        (Cube.Tilt[0] > 1 && mItem == 0 && mOffsetX <= 0)
      ) {
        CoastToStop();
      } else {
        mRestTime = menu.SimTime;

        // accelerate in the direction of tilt
        if (mDrawLabel) {
          Hacky.Sfx("ui_select_02");
        }
        mDrawLabel = false;
        var vSign = Cube.Tilt[0] < 1f ? -1f : 1f;

        // if we're way overtilted (into the upper corners!), accelerate super-fast
        var deepMult = Cube.Tilt[1] == 2 ? DEEPTILTACCEL : 1f;

        if (vSign > 0) {
          if (mItem > 0) {
            if (mOffsetX > 45) { // magic
              mItem--;
              mOffsetX -= 90; // magic
            } else {
              mOffsetX += Resistence * TILTVEL * mAccel;
              mAccel = Mathf.Clamp(mAccel * GRAVITY, MINACCEL, MAXACCEL) * deepMult;
            }
          }
        } else {
          if (mItem < menu.ItemCount-1) {
            if (mOffsetX < -45) { // magic
              mItem++;
              mOffsetX += 90; // magic
            } else {
              mOffsetX -= Resistence * TILTVEL * mAccel;
              mAccel = Mathf.Clamp(mAccel * GRAVITY, MINACCEL, MAXACCEL) * deepMult;
            }
          }
        }
        mDirty = true;
      }
    }

    float Resistence {
      get{
        var u = 1f - Mathf.Abs(mOffsetX / 45f);
        return 1f - 0.95f * (u * u);
      }
    }

    void CoastToStop() {
      if (Mathf.Abs(mOffsetX)  > Mathf.Epsilon) {
        mAccel = Mathf.Clamp(mAccel/GRAVITY, MINACCEL, MAXACCEL);
        mRestTime = menu.SimTime;
        if (Mathf.Abs(mOffsetX) < MINACCEL) { // magic
          StopScrolling();
        } else if (mOffsetX > 0f) {
          mOffsetX -= DRIFTVEL * mAccel;
          if (mOffsetX < 0) { mAccel = MINACCEL; }
        } else {
          mOffsetX += DRIFTVEL * mAccel;
          if (mOffsetX > 0) { mAccel = MINACCEL; }
        }
        mDirty = true;
      }
    }

    void StopScrolling() {
      mOffsetX = 0;
      mAccel = MINACCEL;
      mRestTime = menu.SimTime;
      mDrawLabel = false;
      mDirty = true;
    }

    void ClipIt(int ox, out int x, out int w) {
      x = ox + Mathf.FloorToInt(mOffsetX);
      w = 80;
      if (x < 0) {
        w += x;
        x = 0;
      }
      w -= (x+w>128) ? 80-(128-x) : 0;
    }

    bool PositionOf(Cube c, out Int2 pos) {
      if (Cube == c) {
        pos = Int2.Zero;
        return true;
      }
      if (Cube == null || Cube.Neighbors.IsEmpty) {
        pos = Int2.Zero;
        return false;
      }
      foreach(var view in menu.Views) { view.mVisited = false; }
      mGridPosition = Int2.Zero;
      mVisited = true;
      for(int i=0; i<4; ++i) {
        Int2 p;
        if (PositionOfVisit(this, c, (Side)i, out p)) {
          pos = p;
          return true;
        }
      }
      pos = Int2.Zero;
      return false;
    }

    bool PositionOfVisit(TiltFlowView origin, Cube target, Side s, out Int2 result) {
      var n = origin.Cube.Neighbors[s];
      if (n == null) {
        result = Int2.Zero;
        return false;
      };
      var view = n.userData as TiltFlowView;
      if (view.mVisited) {
        result = Int2.Zero;
        return false;
      }
      view.mVisited = true;
      view.mGridPosition = origin.mGridPosition + Int2.Side(s);
      if (view.Cube == target) {
        result = view.mGridPosition;
        return true;
      } else {
        for(int i=0; i<4; ++i) {
          Int2 p;
          if (PositionOfVisit(view, target, (Side)i, out p)) {
            result = p;
            return true;
          }
        }
        result = Int2.Zero;
        return false;
      }

    }

  }

}


#endif //_TILTFLOWMENU_H
