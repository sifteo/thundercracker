  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

#include "TiltFlowMenu.h"


const float TiltFlowMenu::UPDATE_DELAY = 1.0f / 30.0f;
const float TiltFlowMenu::PICK_DELAY = 2.0f;
const float TiltFlowMenu::REST_DELAY = 0.1f;



TiltFlowMenu::TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes) : mStatus( CHOOSING ), mDone( false ),
    mSimTime( 0.0f ), mUpdateTime( 0.0f ), mPickTime( 0.0f ), mNumCubes( numCubes ), mNumItems( numItems ), mNeighborDirty( true )
{
    //TODO SFX
    //Hacky.Sfx("g_neighborA");

    //TODO ASSIGN VIEWS
    for( int i = 0; i < numCubes; i++ )
        mViews[i].setCube = &Game::Inst()->cubes[i];

    mKeyView = &mViews[0];
    mKeyView->SetStatus(TiltFlowView.STATUS_MENU);
    mKeyView->SetItem(0);

    for( int i = 0; i < numItems; i++ )
        mItems[i] = pItems[i];
}

void TiltFlowMenu::Tick(float dt)
{
    mSimTime += dt;

    if (mDone) {
      // mirr?
    }

    if (mSimTime - mUpdateTime > UPDATE_DELAY) {
      if (mNeighborDirty) {
        mNeighborDirty = false;
        //CheckMenuNeighbors();
      }

      for( int i = 0; i < mNumCubes; i++ )
        mViews[i]->Tick();

      if (mStatus == PICKED && mSimTime - mPickTime > PICK_DELAY) {
        mDone = true;
      }

      mUpdateTime = mSimTime;
    }

    for( int i = 0; i < mNumCubes; i++ )
      mViews[i]->CheckForRepaint();
}


void TiltFlowMenu::Pick(TiltFlowView &view)
{
    if (mStatus == CHOOSING) {
        //TODO SFX
      //Hacky.Sfx("checkpoint2");
      LOG("Selected {0}", view.Item.name);
      mStatus = PICKED;
      mPickTime = mSimTime;
      for( int i = 0; i < mNumCubes; i++ )
      {
          TiltFlowView &v = *mViews[i];
        if (v == view) {
          v.SetStatus(TiltFlowView.STATUS_INFO);
        } else {
          v.SetStatus(TiltFlowView.Status.None);
        }
        v.SetDirty();
      }
    }
}

void TiltFlowMenu::CheckMenuNeighbors() {
  ReassignMenu();
  for( int i = 0; i < mNumCubes; i++ )
  {
      TiltFlowView &view = *mViews[i];
    if (&view == mKeyView) {
      view.SetStatus(TiltFlowView.STATUS_MENU);
      view.SetDirty();
    } else {
      view.RelateToMenu();
    }
  }
}

void TiltFlowMenu::ReassignMenu() {
    //if the keyview is alone
    if( HasNeighbors( mKeyView->Cube ) )
        return;

    //then walk through all the cubes, looking for a new key view
    for( int i = 0; i < mNumCubes; i++ )
    {
        TiltFlowView &view = mViews[i];

        if( view.CurrentStatus == TiltFlowView.STATUS_ITEM )
            continue;

        if( view.ItemIndex < 0 || view.ItemIndex >= mNumItems )
            continue;

        if (mKeyView.mLastNeighborRemoveSide == Side.LEFT)
        {
            if( view.Cube.Neighbors.Right != null || view.Cube.Neighbors.Left == null )
                continue;
        }
        else if (mKeyView.mLastNeighborRemoveSide == Side.RIGHT)
        {
            if( view.Cube.Neighbors.Left != null || view.Cube.Neighbors.Right == null )
                continue;
        }

        //just take the first one
        mKeyView = &view;
    }
}

  //---------------------------------------------------------------------------
  // TILT FLOW ITEM
  //---------------------------------------------------------------------------

TiltFlowItem::TiltFlowItem(Sifteo::AssetImage &image, char *pName/*, string description, Int2 sourcePosition=new Int2()*/) :
    mName( pName ), mImage( image )
{
    //TODO - do we need this stuff?
    /*this.description = description;
    this.color = Color.Mask;
    this.sourcePosition = sourcePosition;*/
}

  //---------------------------------------------------------------------------
  // TILT FLOW VIEW
  //---------------------------------------------------------------------------

const float TiltFlowView::DRIFTVEL = 2.0f;
const float TiltFlowView::TILTVEL = 6.0f;
const float TiltFlowView::MINACCEL = 3.0f;
const float TiltFlowView::MAXACCEL = 8.0f;
const float TiltFlowView::DEEPTILTACCEL = 2.0f;
const float TiltFlowView::GRAVITY = 1.03f;


TiltFlowView::TiltFlowView() :
    mItem( -1 ), mOffsetX( 0.0f ), mAccel( MINACCEL ),
    mRestTime( -1.0f ), mDrawLabel( true ), mDirty( true ),
    mLastNeighborRemoveSide( NONE )
{
}


void TiltFlowView::CheckForRepaint() {
  if (mDirty) {
    Paint();
    mDirty = false;
  }
}

void TiltFlowView::Tick() {
  bool wasDrawingLabel = mDrawLabel;
  switch(mStatus) {
  case STATUS_MENU:
    UpdateMenu();
    break;
  case STATUS_ITEM:
    CoastToStop();
    break;
  default:
    // do nothing
    break;
  }
  if (mRestTime > -1 && menu.SimTime - mRestTime > TiltFlowMenu::REST_DELAY) {
    mDirty = true;
    mDrawLabel = true;
    if (mDrawLabel && !wasDrawingLabel) {
      Hacky.Sfx("ui_select_01");
    }
    mRestTime = -1;
  }
}

void TiltFlowView::Paint(Cube c)
{
    switch(mStatus) {
    case STATUS_MENU:
      PaintMenu(c);
      break;
    case STATUS_ITEM:
      PaintItem(c);
      break;
    case STATUS_INFO:
      PaintInfo(c);
      break;
    default:
      PaintNone(c);
      break;
    }
}


void TiltFlowView::PaintNone(Cube c) {
  if (menu.PaintNone != null) {
    menu.PaintNone(this);
  } else {
    c.FillScreen(Color.White);
    //c.FillScreen(new Color(182, 182, 170)); // magic
    //menu.Font.Paint(c, "-", Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, false, false, new Int2(128,128));
  }
}

void TiltFlowView::PaintInfo(Cube c) {
  if (menu.PaintBackground != null) { menu.PaintBackground(this); } else { c.FillScreen(Color.White); }
  DoPaintItem(Item, 24, 80); // magic
  menu.Font.Paint(c, Item.name, Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128,24)); // magic
  //menu.Font.Paint(c, Item.description, new Int2(0,80), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128,24)); // magic
}

void TiltFlowView::PaintMenu(Cube c) {
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

void TiltFlowView::DoPaintItem(TiltFlowItem item, int x, int w) {
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

void TiltFlowView::PaintItem(Cube c) {
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


void TiltFlowView::OnButton(Cube c, bool pressed) {
  if (menu.CurrentStatus == TiltFlowMenu.CHOOSING && mStatus != Status.None) {
    StopScrolling();
    menu.Pick(this);
    mDirty = true;
  }
}

//TODO need flip
/*
void TiltFlowView::OnFlip(Cube c, bool newOrientationIsUp) {
  if (!newOrientationIsUp) {
    menu.CheckFlip();
  }
}*/

void TiltFlowView::OnNeighborAdd(Cube c, Side s, Cube nc, Side ns) {
  if (menu.CurrentStatus == TiltFlowMenu.CHOOSING) {
    menu.DirtyNeighbors();
  }
}

void TiltFlowView::OnNeighborRemove(Cube c, Side s, Cube nc, Side ns) {
  if (menu.CurrentStatus == TiltFlowMenu.CHOOSING) {
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

