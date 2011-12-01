using Sifteo;
using Sifteo.MathExt;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Sifteo.Util {

  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

  public class TiltFlowMenu : IDisposable {

    public const float kUpdateDelay = 1f/30f;
    public const float kPickDelay = 2;
    public const float kRestDelay = 0.1f;

    public enum Status { Choosing, Picked };

    public readonly BaseApp app;
    readonly List<TiltFlowItem> mItems;
    Status mStatus = Status.Choosing;
    bool mDone = false;
    float mSimTime = 0f;
    float mUpdateTime = 0f;
    float mPickTime = 0f;
    TiltFlowView mKeyView;
    bool mNeighborDirty = false;
    IFont mFont = MenuTest.Library.Verdana;

    public TiltFlowMenu(BaseApp app, params TiltFlowItem[] items) {
      Hacky.Sfx("g_neighborA");
      this.app = app;
      foreach(Cube c in app.CubeSet) {
        DidAddCube(c);
      }

      mKeyView = app.CubeSet[0].userData as TiltFlowView;
      mKeyView.SetStatus(TiltFlowView.Status.Menu);
      mKeyView.SetItem(0);
      mNeighborDirty = true;

      mItems = new List<TiltFlowItem>(items);
      
      app.CubeSet.NewCubeEvent += DidAddCube;
      app.CubeSet.LostCubeEvent += WillLoseCube;
    }

    public void Dispose() {
      foreach(var view in Views) {
        view.Cube = null;
      }
      app.CubeSet.NewCubeEvent -= DidAddCube;
      app.CubeSet.LostCubeEvent -= WillLoseCube;
    }

    public Action<TiltFlowView> PaintNone { internal get; set; }
    public Action<TiltFlowView> PaintBackground { internal get; set; }
    public Action<TiltFlowItemArgs> PaintItem { internal get; set; }

    public IFont Font {
      internal get { return mFont; }
      set { mFont = value; }
    }
    public Status CurrentStatus { get { return mStatus; } }
    public float SimTime { get { return mSimTime; } }

    public TiltFlowView KeyView { get { return mKeyView; } }

    public int ItemCount { get { return mItems.Count; } }
    public TiltFlowItem this[int i] { get { return mItems[i]; } }
    public IEnumerable<TiltFlowItem> Items { get { return mItems; } }

    public IEnumerable<TiltFlowView> Views {
      get {
        for(int i=0; i<app.CubeSet.Count; ++i) {
          yield return app.CubeSet[i].userData as TiltFlowView;
        }
      }
    }

    public bool Done { get { return mDone; } }
    public TiltFlowItem ResultItem { get { return mDone ? mKeyView.Item : null; } }

    public void Tick(float dt) {
      mSimTime += dt;

      if (mDone) {
        // mirr?
      }

      if (mSimTime - mUpdateTime > kUpdateDelay) {
        if (mNeighborDirty) {
          mNeighborDirty = false;
          //CheckMenuNeighbors();
        }

        foreach(var view in Views) {
          view.Tick();
        }

        if (mStatus == Status.Picked && mSimTime - mPickTime > kPickDelay) {
          mDone = true;
          /*
          if (mKeyView.ItemIndex == mItems.Count-1) {
            mDone = true;
          } else {
            mStatus = Status.Choosing;
            foreach(var view in Views) {
              if (view == mKeyView) {
                view.SetStatus(TiltFlowView.Status.Menu);
              } else {
                view.SetStatus(TiltFlowView.Status.None);
              }
              view.SetDirty();
            }
          }
          */
        }

        mUpdateTime = mSimTime;
      }

      foreach(var view in Views) { view.CheckForRepaint(); }
    }

    internal void DirtyNeighbors() {
      mNeighborDirty = true;
    }

    internal void Pick(TiltFlowView view) {
      if (mStatus == Status.Choosing) {
        Hacky.Sfx("checkpoint2");
        Log.Debug("Selected {0}", view.Item.name);
        mStatus = Status.Picked;
        mPickTime = mSimTime;
        foreach(var v in Views) {
          if (v == view) {
            v.SetStatus(TiltFlowView.Status.Info);
          } else {
            v.SetStatus(TiltFlowView.Status.None);
          }
          v.SetDirty();
        }
      }
    }

    internal void CheckFlip() {
      foreach(Cube c in app.CubeSet) {
        if (c.IsUpright) {
          mDone = false;
          return;
        }
      }
      mDone = true;
    }

    void DidAddCube(Cube c) {
      var view = new TiltFlowView(this);
      view.Cube = c;
    }

    void WillLoseCube(Cube c) {
      var view = c.userData as TiltFlowView;
      if (view != null) { view.Cube = null; }
    }

    void CheckMenuNeighbors() {
      ReassignMenu();
      foreach(var view in Views) {
        if (view == mKeyView) {
          view.SetStatus(TiltFlowView.Status.Menu);
          view.SetDirty();
        } else {
          view.RelateToMenu();
        }
      }
    }

    void ReassignMenu() {
      Cube[] keyGroup;
      var groups = FindGroups(out keyGroup);
      groups.RemoveAll(cubeArray => cubeArray.Length == 1);
      if (groups.Count > 0 && keyGroup.Length == 1) {
        // find an Item View at the end of a line
        var oviews = new List<TiltFlowView>(Views);
        oviews.RemoveAll(view => view.CurrentStatus == TiltFlowView.Status.Item);
        if (oviews.Count > 0) {
          if (mKeyView.mLastNeighborRemoveSide == Cube.Side.LEFT) {
            oviews.RemoveAll(view =>
              view.Cube.Neighbors.Right != null ||
              view.Cube.Neighbors.Left == null ||
              view.ItemIndex < 0 ||
              view.ItemIndex >= mItems.Count
            );
          } else {
            oviews.RemoveAll(view =>
              view.Cube.Neighbors.Left != null ||
              view.Cube.Neighbors.Right == null ||
              view.ItemIndex < 0 ||
              view.ItemIndex >= mItems.Count
            );
          }
          if (oviews.Count > 0) {
            mKeyView = oviews[0];
          }
        }
      }
    }

    List<Cube[]> FindGroups(out Cube[] keyGroup) {
      keyGroup = null;
      var result = new List<Cube[]>(app.CubeSet.Count);
      foreach(var view in Views) { view.mVisited = false; }
      Cube nextCube = app.CubeSet[0];
      while(nextCube != null) {
        result.Add(CubeHelper.FindConnected(nextCube));
        foreach(Cube cube in result[result.Count-1]) {
          (cube.userData as TiltFlowView).mVisited = true;
          if (cube == mKeyView.Cube) {
            keyGroup = result[result.Count-1];
          }
        }
        nextCube = null;
        foreach(Cube cube in app.CubeSet) {
          if (!(cube.userData as TiltFlowView).mVisited) {
            nextCube = cube;
            break;
          }
        }
      }
      return result;
    }
  }

  //---------------------------------------------------------------------------
  // TILT FLOW ITEM
  //---------------------------------------------------------------------------

  public struct TiltFlowItemArgs {
    public TiltFlowView view;
    public TiltFlowItem item;
    public AABB bounds;
  }

  public class TiltFlowItem {
    public object userData;
    public readonly Color color;
    public readonly string name;
    public readonly string description;
    public readonly string image;
    public readonly Int2 sourcePosition;

    public TiltFlowItem (Color color, string name, string description) {
      this.color = color;
      this.name = name;
      this.description = description;
      this.image = "";
      this.sourcePosition = Int2.Zero;
    }

    public TiltFlowItem(string image, string name, string description, Int2 sourcePosition=new Int2()) {
      this.image = image;
      this.name = name;
      this.description = description;
      this.color = Color.Mask;
      this.sourcePosition = sourcePosition;
    }

    public bool HasImage {
      get { return image.Length > 0; }
    }
  }

  //---------------------------------------------------------------------------
  // TILT FLOW VIEW
  //---------------------------------------------------------------------------

  public class TiltFlowView : View {
    public const float kDriftVel = 2;
    public const float kTiltVel = 6;
    public const float kMinAccel = 3;
    public const float kMaxAccel = 8;
    public const float kDeepTiltAccel = 2;
    public const float kGravity = 1.03f;
    public enum Status { None, Menu, Item, Info }

    public readonly TiltFlowMenu menu;
    Status mStatus;
    int mItem = -1;
    float mOffsetX = 0;
    float mAccel = kMinAccel;
    float mRestTime = -1;
    bool mDrawLabel = true;
    bool mDirty = true;
    internal Cube.Side mLastNeighborRemoveSide = Cube.Side.NONE;

    // for PositionOf and FindGroups
    internal bool mVisited;
    internal Int2 mGridPosition;

    internal TiltFlowView(TiltFlowMenu menu) {
      this.menu = menu;
    }

    public Status CurrentStatus { get { return mStatus; } }
    public int ItemIndex { get { return mItem; } }
    public TiltFlowItem Item { get { return menu[mItem]; } }

    internal void CheckForRepaint() {
      if (mDirty) {
        Paint();
        mDirty = false;
      }
    }

    internal void SetStatus(Status s) {
      mStatus = s;
    }

    internal void SetDirty() {
      mDirty = true;
    }

    internal void SetItem(int i) {
      mItem = i;
    }

    internal void Tick() {
      var wasDrawingLabel = mDrawLabel;
      switch(mStatus) {
      case Status.Menu:
        UpdateMenu();
        break;
      case Status.Item:
        CoastToStop();
        break;
      default:
        // do nothing
        break;
      }
      if (mRestTime > -1 && menu.SimTime - mRestTime > TiltFlowMenu.kRestDelay) {
        mDirty = true;
        mDrawLabel = true;
        if (mDrawLabel && !wasDrawingLabel) {
          Hacky.Sfx("ui_select_01");
        }
        mRestTime = -1;
      }
    }

    override public void DidAttachToCube(Cube c) {
      c.TiltEvent += OnTilt;
      c.ButtonEvent += OnButton;
      c.FlipEvent += OnFlip;
      c.NeighborAddEvent += OnNeighborAdd;
      c.NeighborRemoveEvent += OnNeighborRemove;
    }

    override public void WillDetachFromCube(Cube c) {
      c.TiltEvent -= OnTilt;
      c.ButtonEvent -= OnButton;
      c.FlipEvent -= OnFlip;
      c.NeighborAddEvent -= OnNeighborAdd;
      c.NeighborRemoveEvent -= OnNeighborRemove;
    }

    override public void Paint(Cube c) {
      switch(mStatus) {
      case Status.Menu:
        PaintMenu(c);
        break;
      case Status.Item:
        PaintItem(c);
        break;
      case Status.Info:
        PaintInfo(c);
        break;
      default:
        PaintNone(c);
        break;
      }
    }

    void PaintNone(Cube c) {
      if (menu.PaintNone != null) {
        menu.PaintNone(this);
      } else {
        c.FillScreen(Color.White);
        //c.FillScreen(new Color(182, 182, 170)); // magic
        //menu.Font.Paint(c, "-", Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, false, false, new Int2(128,128));
      }
    }

    void PaintInfo(Cube c) {
      if (menu.PaintBackground != null) { menu.PaintBackground(this); } else { c.FillScreen(Color.White); }
      DoPaintItem(Item, 24, 80); // magic
      menu.Font.Paint(c, Item.name, Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128,24)); // magic
      //menu.Font.Paint(c, Item.description, new Int2(0,80), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128,24)); // magic
    }

    void PaintMenu(Cube c) {
      if (menu.PaintBackground != null) { menu.PaintBackground(this); } else { c.FillScreen(Color.White); }
      int x, w;
      ClipIt(24, out x, out w); // magic
      if (w > 0) { DoPaintItem(Item, x, w); }
      if (c.Neighbors.Left == null && mItem > 0) {
        ClipIt(-70, out x, out w); // magic
        if (w > 0) { DoPaintItem(menu[mItem-1], x, w); }
      }
      if (c.Neighbors.Right == null && mItem < menu.ItemCount-1) {
        ClipIt(118, out x, out w); // magic
        if (w > 0) { DoPaintItem(menu[mItem+1], x, 80); }
      }
      if (mDrawLabel) {
        if (c.Neighbors.Left == null && c.Neighbors.Right == null) {
          if (mItem < menu.ItemCount-1) {
            c.Image("gestures", 4, 99, 0, 192, 27, 25); // magic
            //c.Image("curlyarrows", 4, 108, 0, 16, 16, 16); // magic
          }
          if (mItem > 0) {
            c.Image("gestures", 97, 99, 0, 28, 27, 25); // magic
            //c.Image("curlyarrows", 108, 108, 0, 0, 16, 16); // magic
          }
          var g = new AABB(0, 0, 24, 28); // magic, taken from GESTURE_ICONS table in original python source
          c.Image("gestures", 64-g.size.x/2, 128-4-g.size.y, g.position.x, g.position.y, g.size.x, g.size.y, 1, 0); // magic
        }
        menu.Font.Paint(c, Item.name, Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128, 20)); // magic
      }
    }

    void DoPaintItem(TiltFlowItem item, int x, int w) {
      const int y = 24; // magic
      const int h = 80; // magic
      if (menu.PaintItem != null) {
        menu.PaintItem(new TiltFlowItemArgs() {
          view = this,
          item = item,
          bounds = x==0 && w<80 ? new AABB(w-80, y, h, h) : new AABB(x, y, h, h)
        });
      }
      if (item.HasImage) {
        if (x==0 && w<80) {
          Cube.Image(item.image, w-80, y, item.sourcePosition.x, item.sourcePosition.y, 80, h, 1, 0);
        } else {
          Cube.Image(item.image, x, y, item.sourcePosition.x, item.sourcePosition.y, w, h, 1, 0);
        }
      } else {
        Cube.FillRect(item.color, x, y, w, h);
      }
    }

    void PaintItem(Cube c) {
      if (menu.PaintBackground != null) { menu.PaintBackground(this); } else { c.FillScreen(Color.White); }
      int x, w;
      ClipIt(24, out x, out w); // magic
      if (w > 0) {
        DoPaintItem(Item, x, w);
        if (mDrawLabel) {
          menu.Font.Paint(c, Item.name, Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128,20)); // magic
        }
      }
    }

    void OnTilt(Cube c, int x, int y, int z) {
      // pass
    }

    void OnButton(Cube c, bool pressed) {
      if (menu.CurrentStatus == TiltFlowMenu.Status.Choosing && mStatus != Status.None) {
        StopScrolling();
        menu.Pick(this);
        mDirty = true;
      }
    }

    void OnFlip(Cube c, bool newOrientationIsUp) {
      if (!newOrientationIsUp) {
        menu.CheckFlip();
      }
    }

    void OnNeighborAdd(Cube c, Cube.Side s, Cube nc, Cube.Side ns) {
      if (menu.CurrentStatus == TiltFlowMenu.Status.Choosing) {
        menu.DirtyNeighbors();
      }
    }

    void OnNeighborRemove(Cube c, Cube.Side s, Cube nc, Cube.Side ns) {
      if (menu.CurrentStatus == TiltFlowMenu.Status.Choosing) {
        mLastNeighborRemoveSide = s;
        menu.DirtyNeighbors();
      }
    }

    internal void RelateToMenu() {
      var oldStatus = mStatus;
      var oldItem = mItem;
      Int2 pos;
      if (menu.KeyView.PositionOf(Cube, out pos)) {
        if (pos.y == 0) {
          mItem = menu.KeyView.mItem + pos.x;
          if (mItem >= 0 && mItem < menu.ItemCount) {
            if (mStatus != Status.Item) {
              mStatus = Status.Item;
              mAccel = kMaxAccel;
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
            mStatus = Status.Info;
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
        var deepMult = Cube.Tilt[1] == 2 ? kDeepTiltAccel : 1f;

        if (vSign > 0) {
          if (mItem > 0) {
            if (mOffsetX > 45) { // magic
              mItem--;
              mOffsetX -= 90; // magic
            } else {
              mOffsetX += Resistence * kTiltVel * mAccel;
              mAccel = Mathf.Clamp(mAccel * kGravity, kMinAccel, kMaxAccel) * deepMult;
            }
          }
        } else {
          if (mItem < menu.ItemCount-1) {
            if (mOffsetX < -45) { // magic
              mItem++;
              mOffsetX += 90; // magic
            } else {
              mOffsetX -= Resistence * kTiltVel * mAccel;
              mAccel = Mathf.Clamp(mAccel * kGravity, kMinAccel, kMaxAccel) * deepMult;
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
        mAccel = Mathf.Clamp(mAccel/kGravity, kMinAccel, kMaxAccel);
        mRestTime = menu.SimTime;
        if (Mathf.Abs(mOffsetX) < kMinAccel) { // magic
          StopScrolling();
        } else if (mOffsetX > 0f) {
          mOffsetX -= kDriftVel * mAccel;
          if (mOffsetX < 0) { mAccel = kMinAccel; }
        } else {
          mOffsetX += kDriftVel * mAccel;
          if (mOffsetX > 0) { mAccel = kMinAccel; }
        }
        mDirty = true;
      }
    }

    void StopScrolling() {
      mOffsetX = 0;
      mAccel = kMinAccel;
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
        if (PositionOfVisit(this, c, (Cube.Side)i, out p)) {
          pos = p;
          return true;
        }
      }
      pos = Int2.Zero;
      return false;
    }

    bool PositionOfVisit(TiltFlowView origin, Cube target, Cube.Side s, out Int2 result) {
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
          if (PositionOfVisit(view, target, (Cube.Side)i, out p)) {
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

