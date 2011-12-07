  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

#include "TiltFlowMenu.h"
#include "game.h"
#include "cubewrapper.h"
#include "assets.gen.h"

const float TiltFlowMenu::UPDATE_DELAY = 1.0f / 30.0f;
const float TiltFlowMenu::PICK_DELAY = 2.0f;
const float TiltFlowMenu::REST_DELAY = 0.1f;


TiltFlowMenu *TiltFlowMenu::s_pInst = NULL;


TiltFlowMenu *TiltFlowMenu::Inst()
{
    return s_pInst;
}

TiltFlowMenu::TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes) : mStatus( CHOOSING ), mDone( false ),
    mSimTime( 0.0f ), mUpdateTime( 0.0f ), mPickTime( 0.0f ), mNumCubes( numCubes ), mNumItems( numItems ), mNeighborDirty( true )
{
    s_pInst = this;
    //TODO SFX
    //Hacky.Sfx("g_neighborA");

    mKeyView = &mViews[0];
    mKeyView->SetStatus(TiltFlowView::STATUS_MENU);
    mKeyView->SetItem(0);

    mItems = pItems;
}


void TiltFlowMenu::AssignViews()
{
    //TODO ASSIGN VIEWS
    for( int i = 0; i < mNumCubes; i++ )
        mViews[i].SetCube( &Game::Inst().cubes[i].GetCube() );
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
        mViews[i].Tick();

      if (mStatus == PICKED && mSimTime - mPickTime > PICK_DELAY) {
        mDone = true;
      }

      mUpdateTime = mSimTime;
    }

    for( int i = 0; i < mNumCubes; i++ )
      mViews[i].CheckForRepaint();
}


void TiltFlowMenu::Pick(TiltFlowView &view)
{
    if (mStatus == CHOOSING) {
        //TODO SFX
      //Hacky.Sfx("checkpoVec2");
      //LOG(("Selected {0}", view.Item.name));
      mStatus = PICKED;
      mPickTime = mSimTime;
      for( int i = 0; i < mNumCubes; i++ )
      {
          TiltFlowView &v = mViews[i];
        if (&v == &view) {
          v.SetStatus(TiltFlowView::STATUS_INFO);
        } else {
          v.SetStatus(TiltFlowView::STATUS_NONE);
        }
        v.SetDirty();
      }
    }
}


TiltFlowItem *TiltFlowMenu::GetItem( int item )
{
    ASSERT( item >= 0 && item < mNumItems );

    if( item >= 0 && item < mNumItems )
        return &mItems[ item ];

    return NULL;
}



/*
  //TODO, THIS IS NEIGHBORING, NOT HAPPENING YET
void TiltFlowMenu::CheckMenuNeighbors() {
  ReassignMenu();
  for( int i = 0; i < mNumCubes; i++ )
  {
      TiltFlowView &view = mViews[i];
    if (&view == mKeyView) {
      view.SetStatus(TiltFlowView::STATUS_MENU);
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

        if( view.GetStatus() == TiltFlowView::STATUS_ITEM )
            continue;

        if( view.ItemIndex < 0 || view.ItemIndex >= mNumItems )
            continue;

        if (mKeyView.mLastNeighborRemoveSide == Side.LEFT)
        {
            if( view.Cube.Neighbors.Right != NULL || view.Cube.Neighbors.Left == NULL )
                continue;
        }
        else if (mKeyView.mLastNeighborRemoveSide == Side.RIGHT)
        {
            if( view.Cube.Neighbors.Left != NULL || view.Cube.Neighbors.Right == NULL )
                continue;
        }

        //just take the first one
        mKeyView = &view;
    }
}*/

  //---------------------------------------------------------------------------
  // TILT FLOW ITEM
  //---------------------------------------------------------------------------

TiltFlowItem::TiltFlowItem(const Sifteo::AssetImage &image, const char *pName/*, string description, Vec2 sourcePosition=new Vec2()*/) :
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
//const float TiltFlowView::TILTVEL = 0.6f;
const float TiltFlowView::MINACCEL = 3.0f;
const float TiltFlowView::MAXACCEL = 8.0f;
const float TiltFlowView::DEEPTILTACCEL = 2.0f;
const float TiltFlowView::GRAVITY = 1.03f;
const float TiltFlowView::EPSILON = 0.00001f;


int TiltFlowView::s_cubeIndex = 0;


TiltFlowView::TiltFlowView() :
    mItem( -1 ), mOffsetX( 0.0f ), mAccel( MINACCEL ),
    mRestTime( -1.0f ), mDrawLabel( true ), mDirty( true ),
    mLastNeighborRemoveSide( NONE ), mpCube( NULL )
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

  if (mRestTime > -1 && TiltFlowMenu::Inst()->GetSimTime() - mRestTime > TiltFlowMenu::REST_DELAY) {
    mDirty = true;
    mDrawLabel = true;

    //TODO SFX
    /*if (mDrawLabel && !wasDrawingLabel) {
      Hacky.Sfx("ui_select_01");
    }*/
    mRestTime = -1;
  }
}

void TiltFlowView::Paint()
{
    switch(mStatus) {
    case STATUS_MENU:
      PaintMenu();
      break;
    case STATUS_ITEM:
      PaintItem();
      break;
    case STATUS_INFO:
      PaintInfo();
      break;
    default:
      PaintNone();
      break;
    }
}


void TiltFlowView::PaintNone() {
  /*if (TiltFlowMenu::Inst()->PaintNone != NULL) {
    TiltFlowMenu::Inst()->PaintNone(this);
  } else {
    c.FillScreen(Color.White);
  }*/
}

void TiltFlowView::PaintInfo() {
   /*
  if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }
  DoPaintItem(Item, 24, 80); // magic
  TiltFlowMenu::Inst()->Font.Paint(c, Item.name, Vec2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Vec2(128,24)); // magic
  */
}

void TiltFlowView::PaintMenu() {
  //if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }

  //could be better, but just clear for now
  VidMode_BG0 vid( mpCube->vbuf );
  vid.clear(Font.tiles[0]);

  //DEBUG DRAW
  /*for( unsigned int i = 0; i < VidMode_BG0::BG0_width; i++ )
  {
      vid.BG0_textf( Vec2( i, 0 ), Font, "%d", i%10 );
  }*/

  DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem ), 24);

  if (/*c.Neighbors.Left == NULL && */mItem > 0) {
    DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem - 1 ), -70);
  }
  if (/*c.Neighbors.Right == NULL && */mItem < TiltFlowMenu::Inst()->GetNumItems()-1) {
    DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem + 1), 118);
  }

  /*if (mDrawLabel) {
    if (c.Neighbors.Left == NULL && c.Neighbors.Right == NULL) {
      if (mItem < TiltFlowMenu::Inst()->GetNumItems()-1) {
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
    TiltFlowMenu::Inst()->Font.Paint(c, Item.name, Vec2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Vec2(128, 20)); // magic
  }*/

  // Firmware handles all pixel-level scrolling
  vid.BG0_setPanning(Vec2((int)-mOffsetX, 0));
}

void TiltFlowView::DoPaintItem(TiltFlowItem *pItem, int x/*, int w*/) {
  //const int y = 24; // magic
  //const int h = 80; // magic
  /*if (TiltFlowMenu::Inst()->PaintItem != NULL) {
    TiltFlowMenu::Inst()->PaintItem(new TiltFlowItemArgs() {
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
  }*/


  if( pItem )
  {
      VidMode_BG0 vid( mpCube->vbuf );
      Vec2 offset = Vec2( 0, 0 );
      Vec2 size = Vec2( pItem->mImage.width, pItem->mImage.height );

      int leftMostTile = -mOffsetX / 8 - 1;
      int rightMostTile = leftMostTile + VidMode_BG0::BG0_width - 1;
      int curTile = x / 8;

      if( curTile > rightMostTile || curTile + (int)pItem->mImage.width < leftMostTile )
          return;

      if( curTile < leftMostTile )
      {
          offset.x = leftMostTile-curTile;
          size.x -= offset.x;
          curTile = leftMostTile;
      }
      else if( curTile + pItem->mImage.width > (unsigned)rightMostTile + 1 )
      {
          size.x -= curTile + pItem->mImage.width - rightMostTile;
      }

      ASSERT( size.x >= 0 );

      //this shouldn't ever really loop, since we're heavily constraining curTile
      while( curTile < 0 )
      {
          curTile += VidMode_BG0::BG0_width;
      }

      //need to draw on the right tiles.
      if( curTile + size.x < (int)VidMode_BG0::BG0_width )
        vid.BG0_drawPartialAsset(Vec2(curTile,3), offset, size, pItem->mImage, 0);
      //Handle wrapping
      else
      {
          int wrapAmt = curTile + size.x - VidMode_BG0::BG0_width;
          size.x -= wrapAmt;
          vid.BG0_drawPartialAsset(Vec2(curTile,3), offset, size, pItem->mImage, 0);
          offset.x += size.x;
          size.x = wrapAmt;
          vid.BG0_drawPartialAsset(Vec2(0,3), offset, size, pItem->mImage, 0);
      }
  }
}

void TiltFlowView::PaintItem() {
  /*if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }
  int x, w;
  ClipIt(24, out x, out w); // magic
  if (w > 0) {
    DoPaintItem(Item, x, w);
    if (mDrawLabel) {
      TiltFlowMenu::Inst()->Font.Paint(c, Item.name, Vec2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Vec2(128,20)); // magic
    }
  }*/
}


void TiltFlowView::OnButton(bool pressed) {
  if (TiltFlowMenu::Inst()->GetStatus() == TiltFlowMenu::CHOOSING && mStatus != STATUS_NONE) {
    StopScrolling();
    TiltFlowMenu::Inst()->Pick(*this);
    mDirty = true;
  }
}


//TODO need flip
/*
void TiltFlowView::OnFlip(Cube c, bool newOrientationIsUp) {
  if (!newOrientationIsUp) {
    TiltFlowMenu::Inst()->CheckFlip();
  }
}*/


//TODO, remove neighboring for now
/*
void TiltFlowView::OnNeighborAdd(Cube c, Side s, Cube nc, Side ns) {
  if (TiltFlowMenu::Inst()->GetStatus() == TiltFlowTiltFlowMenu::Inst()->CHOOSING) {
    TiltFlowMenu::Inst()->DirtyNeighbors();
  }
}

void TiltFlowView::OnNeighborRemove(Cube c, Side s, Cube nc, Side ns) {
  if (TiltFlowMenu::Inst()->GetStatus() == TiltFlowTiltFlowMenu::Inst()->CHOOSING) {
    mLastNeighborRemoveSide = s;
    TiltFlowMenu::Inst()->DirtyNeighbors();
  }
}

void TiltFlowView::RelateToMenu() {
  Status oldStatus = mStatus;
  int oldItem = mItem;
  Vec2 pos;
  if (TiltFlowMenu::Inst()->GetKeyView()->PositionOf(Cube, out pos)) {
    if (pos.y == 0) {
      mItem = TiltFlowMenu::Inst()->GetKeyView()->mItem + pos.x;
      if (mItem >= 0 && mItem < TiltFlowMenu::Inst()->GetNumItems()) {
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
        mStatus = STATUS_NONE;
      }
    } else if (pos.y == -1 || pos.y == 1) {
      mItem = TiltFlowMenu::Inst()->GetKeyView()->mItem + pos.x;
      if (mItem >= 0 && mItem < TiltFlowMenu::Inst()->GetNumItems()) {
        mStatus = STATUS_INFO;
        mOffsetX = 0;
      } else {
        mStatus = STATUS_NONE;
      }
    } else {
      mStatus = STATUS_NONE;
    }
  } else {
    mStatus = STATUS_NONE;
  }

  //TODO SOUND
  if (mItem != oldItem || mStatus != oldStatus) {
    Hacky.Sfx("ui_select_02");
  }
  mDirty |= mStatus != oldStatus;
  mDirty |= mItem != oldItem;
}
  */

void TiltFlowView::UpdateMenu() {
  Cube::TiltState state = mpCube->getTiltState();

  //TEMP DEBUG, use neighboring instead of tilt
  /*if( mpCube->hasPhysicalNeighborAt(RIGHT) )
      state.x = _SYS_TILT_POSITIVE;
  else if( mpCube->hasPhysicalNeighborAt(LEFT) )
      state.x = _SYS_TILT_NEGATIVE;
  else
      state.x = _SYS_TILT_NEUTRAL;
*/

  if (
    state.x == _SYS_TILT_NEUTRAL ||
    (state.x == _SYS_TILT_NEGATIVE && mItem == TiltFlowMenu::Inst()->GetNumItems()-1 && mOffsetX >= 0) ||
    (state.x == _SYS_TILT_POSITIVE && mItem == 0 && mOffsetX <= 0)
  ) {
    CoastToStop();
  } else {
    mRestTime = TiltFlowMenu::Inst()->GetSimTime();

    // accelerate in the direction of tilt
    //TODO SOUND
    /*if (mDrawLabel) {
      Hacky.Sfx("ui_select_02");
    }*/
    mDrawLabel = false;
    float vSign = state.x == _SYS_TILT_NEGATIVE ? -1.0f : 1.0f;

    // if we're way overtilted (into the upper corners!), accelerate super-fast
    float deepMult = state.y == _SYS_TILT_POSITIVE ? DEEPTILTACCEL : 1.0f;

    if (vSign > 0) {
      if (mItem > 0) {
        if (mOffsetX > 45) { // magic
          mItem--;
          mOffsetX -= 90; // magic
        } else {
            mOffsetX += Resistance() * TILTVEL * mAccel;
          mAccel = Util::Clamp(mAccel * GRAVITY, MINACCEL, MAXACCEL) * deepMult;
        }
      }
    } else {
      if (mItem < TiltFlowMenu::Inst()->GetNumItems()-1) {
        if (mOffsetX < -45) { // magic
          mItem++;
          mOffsetX += 90; // magic
        } else {
            mOffsetX -= Resistance() * TILTVEL * mAccel;
          mAccel = Util::Clamp(mAccel * GRAVITY, MINACCEL, MAXACCEL) * deepMult;
        }
      }
    }
    mDirty = true;
  }
}

float TiltFlowView::Resistance() {
    float u = 1.0f - fabs(mOffsetX / 45.0f);
    return 1.0f - 0.95f * (u * u);
}

void TiltFlowView::CoastToStop() {
  if (fabs(mOffsetX)  > EPSILON ) {
    mAccel = Util::Clamp(mAccel/GRAVITY, MINACCEL, MAXACCEL);
    mRestTime = TiltFlowMenu::Inst()->GetSimTime();
    if (fabs(mOffsetX) < MINACCEL) { // magic
      StopScrolling();
    } else if (mOffsetX > 0.0f) {
          mOffsetX -= DRIFTVEL * mAccel;
          if (mOffsetX < 0) { mAccel = MINACCEL; }
    } else {
          mOffsetX += DRIFTVEL * mAccel;
          if (mOffsetX > 0) { mAccel = MINACCEL; }
    }
    mDirty = true;
  }
}

void TiltFlowView::StopScrolling() {
  mOffsetX = 0;
  mAccel = MINACCEL;
  mRestTime = TiltFlowMenu::Inst()->GetSimTime();
  mDrawLabel = false;
  mDirty = true;
}

/*
  TODO
  neighboring doesn't happen for tiltflow now

bool PositionOf(Cube c, out Vec2 pos) {
  if (Cube == c) {
    pos = Vec2.Zero;
    return true;
  }
  if (Cube == NULL || Cube.Neighbors.IsEmpty) {
    pos = Vec2.Zero;
    return false;
  }
  foreach(var view in TiltFlowMenu::Inst()->Views) { view.mVisited = false; }
  mGridPosition = Vec2.Zero;
  mVisited = true;
  for(int i=0; i<4; ++i) {
    Vec2 p;
    if (PositionOfVisit(this, c, (Side)i, out p)) {
      pos = p;
      return true;
    }
  }
  pos = Vec2.Zero;
  return false;
}

bool PositionOfVisit(TiltFlowView origin, Cube target, Side s, out Vec2 result) {
  var n = origin.Cube.Neighbors[s];
  if (n == NULL) {
    result = Vec2.Zero;
    return false;
  };
  var view = n.userData as TiltFlowView;
  if (view.mVisited) {
    result = Vec2.Zero;
    return false;
  }
  view.mVisited = true;
  view.mGridPosition = origin.mGridPosition + Vec2.Side(s);
  if (view.Cube == target) {
    result = view.mGridPosition;
    return true;
  } else {
    for(int i=0; i<4; ++i) {
      Vec2 p;
      if (PositionOfVisit(view, target, (Side)i, out p)) {
        result = p;
        return true;
      }
    }
    result = Vec2.Zero;
    return false;
  }

}

}

*/


