  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

#include "TiltFlowMenu.h"
#include "MenuController.h"
#include "TFcubewrapper.h"
#include "assets.gen.h"
#include "audio.gen.h"

using namespace SelectorMenu;

const float TiltFlowMenu::UPDATE_DELAY = 1.0f / 30.0f;
const float TiltFlowMenu::PICK_DELAY = 2.0f;
const float TiltFlowMenu::REST_DELAY = 0.1f;


TiltFlowMenu *TiltFlowMenu::s_pInst = NULL;


TiltFlowMenu *TiltFlowMenu::Inst()
{
    return s_pInst;
}

TiltFlowMenu::TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes) : mStatus( CHOOSING ), 
    mSimTime( 0.0f ), mUpdateTime( 0.0f ), mPickTime( 0.0f ), mNumCubes( numCubes ), mNumItems( numItems ), 
	mDone( false ), mNeighborDirty( true )
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
        mViews[i].SetCube( &MenuController::Inst().cubes[i].GetCube() );

	//for some reason this doesn't work if it's called in the constructor, so put it here for now.
	m_SFXChannel.init();
}


bool TiltFlowMenu::Tick(float dt)
{
    mSimTime += dt;

    if (mDone) {
      return false;
    }

    if (mSimTime - mUpdateTime > UPDATE_DELAY) {

      //CES HACKERY
	  for( int i = 0; i < mNumCubes; i++ )
	  {
			TiltFlowView &view = mViews[i];
			if (&view == mKeyView) {
				_SYSCubeID neighbor = view.getNeighbor();

				if( neighbor != CUBE_ID_UNDEFINED )
				{
					Pick( view );
					TiltFlowView &neighborView = mViews[neighbor];

					neighborView.setUpIncomingCover( view );
				}

				break;
			}
	  }

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

	return true;
}


void TiltFlowMenu::Pick(TiltFlowView &view)
{
    if (mStatus == CHOOSING) {
      TiltFlowMenu::Inst()->playSound( select );
      //LOG(("Selected {0}", view.Item.name));
      mStatus = PICKED;
      mPickTime = mSimTime;
      for( int i = 0; i < mNumCubes; i++ )
      {
          TiltFlowView &v = mViews[i];
        if (&v == &view) {
          //v.SetStatus(TiltFlowView::STATUS_INFO);
			v.SetStatus(TiltFlowView::STATUS_PICKED);
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

	

//TODO, THIS IS NEIGHBORING, NOT HAPPENING YET
/*
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


void TiltFlowMenu::playSound( const _SYSAudioModule &sound )
{
    m_SFXChannel.stop();
    m_SFXChannel.play(sound, LoopOnce);
}

  //---------------------------------------------------------------------------
  // TILT FLOW ITEM
  //---------------------------------------------------------------------------

/*
TiltFlowItem::TiltFlowItem(const Sifteo::AssetImage &image, const char *pName) :
    mName( pName ), mImage( image )
{
}*/

TiltFlowItem::TiltFlowItem(const Sifteo::AssetImage &image, const Sifteo::AssetImage &label) :
    mImage( image ), mLabel( label )
{
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
const float TiltFlowView::TIP_DELAY = 2.0f;

const Sifteo::AssetImage *TiltFlowView::TIPS[NUM_TIPS] =
{
    &Tip1,
    &Tip2,
    &Tip3,
};

int TiltFlowView::s_cubeIndex = 0;


TiltFlowView::TiltFlowView() :
    mItem( -1 ), mOffsetX( 0.0f ), mAccel( MINACCEL ),
    mRestTime( -1.0f ), mDrawLabel( true ), mDirty( true ),
    mCurrentTip( 0 ), mTipTime(0.0f), mLastNeighborRemoveSide( NONE ), mpCube( NULL ),
	mLastNeighboredSide( NONE )
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
  case STATUS_PICKED:
  case STATUS_STARTING:
    mDirty = true;
    break;
  default:
    // do nothing
    break;
  }

  if (mRestTime > -1 && TiltFlowMenu::Inst()->GetSimTime() - mRestTime > TiltFlowMenu::REST_DELAY) {
    mDirty = true;
    mDrawLabel = true;

    if (mDrawLabel && !wasDrawingLabel) {
      TiltFlowMenu::Inst()->playSound( settle );
    }
    mRestTime = -1;
  }

  if (TiltFlowMenu::Inst()->GetSimTime() - mTipTime > TIP_DELAY)
  {
      mCurrentTip = ( mCurrentTip + 1 ) % NUM_TIPS;
      mTipTime = TiltFlowMenu::Inst()->GetSimTime();
      mDirty = true;
  }
}


_SYSCubeID TiltFlowView::getNeighbor()
{
	for( int i = 0; i < NUM_SIDES; i++ )
	{
		if( mpCube->hasPhysicalNeighborAt(i) )
		{
			mLastNeighboredSide = (Side)i;
			return mpCube->physicalNeighborAt(i);
		}
	}

	return CUBE_ID_UNDEFINED;
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
	case STATUS_PICKED:
      PaintMenu();
      break;
	case STATUS_STARTING:
      PaintIncomingCover();
      break;
    default:
      PaintNone();
      break;
    }
}


void TiltFlowView::PaintNone() {
  VidMode_BG0 vid( mpCube->vbuf );
  vid.clear(White.tiles[0]);
}

void TiltFlowView::PaintInfo() {
   /*
  if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }
  DoPaintItem(Item, 24, 80); // magic
  TiltFlowMenu::Inst()->Font.Paint(c, Item.name, Vec2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Vec2(128,24)); // magic
  */
}

const float SCROLL_AMOUNT = 220.0f;

Float2 SCROLL_DIRS[] = {
	Float2( 0.0f, SCROLL_AMOUNT ),
	Float2( SCROLL_AMOUNT, 0.0f ),
	Float2( 0.0f, -SCROLL_AMOUNT ),
	Float2( -SCROLL_AMOUNT, 0.0f ),
};

void TiltFlowView::PaintMenu() {
  //if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }

  //could be better, but just clear for now
  VidMode_BG0 vid( mpCube->vbuf );
  vid.clear(White.tiles[0]);

  //DEBUG DRAW
  /*for( unsigned int i = 0; i < VidMode_BG0::BG0_width; i++ )
  {
      vid.BG0_textf( Vec2( i, 0 ), Font, "%d", i%10 );
  }*/

  Vec2 offset( 24, 0 );

  if( mStatus == STATUS_PICKED )
  {
	  //scroll the cover off
	  offset -= Vec2( SCROLL_DIRS[ mLastNeighboredSide ] * TiltFlowMenu::Inst()->GetScrollTime() / TiltFlowMenu::PICK_DELAY );
	  //printf( "panning %d, %d.  Pan offset : %0.2f\n", offset.x, offset.y, ( SCROLL_DIRS[ mLastNeighboredSide ] * TiltFlowMenu::Inst()->GetScrollTime() / TiltFlowMenu::PICK_DELAY ).x );
  }


  DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem ), offset.x, offset.y);
  BG1Helper &bg1helper = MenuController::Inst().cubes[ mpCube->id() ].GetBG1Helper();

  if( mStatus != STATUS_PICKED )
  {
	  if (/*c.Neighbors.Left == NULL && */mItem > 0) {
		DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem - 1 ), -70);
	  }
	  if (/*c.Neighbors.Right == NULL && */mItem < TiltFlowMenu::Inst()->GetNumItems()-1) {
		DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem + 1), 118);
	  }

	  if (mDrawLabel) {
		//TiltFlowMenu::Inst()->Font.Paint(c, Item.name, Vec2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Vec2(128, 20)); // magic
		  bg1helper.DrawAsset(Vec2(LABEL_OFFSET,0), TiltFlowMenu::Inst()->GetItem( mItem )->mLabel);
	  }

	  //draw the current tip
	  bg1helper.DrawAsset(Vec2(0,TIP_Y_OFFSET), *TIPS[mCurrentTip]);
  }

  bg1helper.Flush();

  // Firmware handles all pixel-level scrolling
  vid.BG0_setPanning(Vec2((int)-mOffsetX, COVER_Y_OFFSET - offset.y));
}

void TiltFlowView::DoPaintItem(TiltFlowItem *pItem, int x, int y) {
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

	  //y axis handled completely differently, since we aren't panning on y
	  //just clip y to 0 and bottom of screen
	  //CES HACKERY
	  int yTile = y / 8;
	  int yOffset = 0;

	  if( yTile + size.y <= -3 )
		  return;
	  else if( yTile < -3 )
	  {
		  int diff = -3 - yTile;
		  offset.y += diff;
		  size.y -= diff;
		  yOffset += diff;
	  }
	  else if( yTile + pItem->mImage.height > VidMode_BG0::BG0_height - 4 )
	  {
		  int diff = ( yTile + pItem->mImage.height ) - ( VidMode_BG0::BG0_height - 4 );

		  if( diff >= size.y )
			  return;
		  size.y -= diff;
	  }

      //this shouldn't ever really loop, since we're heavily constraining curTile
      while( curTile < 0 )
      {
          curTile += VidMode_BG0::BG0_width;
      }

      //need to draw on the right tiles.
      if( curTile + size.x < (int)VidMode_BG0::BG0_width )
        vid.BG0_drawPartialAsset(Vec2(curTile,yOffset), offset, size, pItem->mImage, 0);
      //Handle wrapping
      else
      {
          int wrapAmt = curTile + size.x - VidMode_BG0::BG0_width;
          size.x -= wrapAmt;
          vid.BG0_drawPartialAsset(Vec2(curTile,yOffset), offset, size, pItem->mImage, 0);
          offset.x += size.x;
          size.x = wrapAmt;
          vid.BG0_drawPartialAsset(Vec2(0,yOffset), offset, size, pItem->mImage, 0);
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


Vec2 TiltFlowView::LerpPosition( const Vec2 &start, const Vec2 &end, float timePercent )
{
	if( timePercent < 0.0f )
		timePercent = 0.0f;
	if( timePercent > 1.0f )
		timePercent = 1.0f;
    Vec2 result( start.x + ( end.x - start.x ) * timePercent, start.y + ( end.y - start.y ) * timePercent );

    return result;
}

static const int OFFSCREEN_POS = 32;

static const Vec2 STARTING_PT[] = {
	Vec2( 0, -OFFSCREEN_POS ),
	Vec2( -OFFSCREEN_POS, 0 ),
	Vec2( 0, OFFSCREEN_POS ),
	Vec2( OFFSCREEN_POS, 0 ),
};

void TiltFlowView::PaintIncomingCover()
{
	//BG1Helper &bg1helper = MenuController::Inst().cubes[ mpCube->id() ].GetBG1Helper();
	VidMode_BG0 vid( mpCube->vbuf );
	vid.BG0_setPanning(Vec2(0, 0));
	//bg1helper.Flush();

	int startIndex = mLastNeighboredSide - mpCube->orientation();

	if( startIndex < 0 )
		startIndex += 4;

	startIndex %= 4;

	Vec2 pos = LerpPosition( STARTING_PT[startIndex], Vec2( 0, 0 ), TiltFlowMenu::Inst()->GetScrollTime() / ( TiltFlowMenu::PICK_DELAY - 0.5f ) );
	Vec2 size( 16, 16 );
	Vec2 offset( 0, 0 );

	if( pos.x <= -16 || pos.x >= 16 )
		return;
	else if( pos.x < 0 )
	{
		offset.x -= pos.x;
		size.x += pos.x;
		pos.x = 0;
	}
	else if( pos.x > 0 )
	{
		size.x -= pos.x;
	}

	if( pos.y <= -16 || pos.y >= 16 )
		return;
	else if( pos.y < 0 )
	{
		offset.y -= pos.y;
		size.y += pos.y;
		pos.y = 0;
	}
	else if( pos.y > 0 )
	{
		size.y -= pos.y;
	}

	//printf( "Attempting to draw at %d, %d.  Offset %d, %d, size %d, %d\n", pos.x, pos.y, offset.x, offset.y, size.x, size.y ); 

	vid.BG0_drawPartialAsset(pos, offset, size, ChromaCover, 0);
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
    if (mDrawLabel) {
      TiltFlowMenu::Inst()->playSound( changeoption );
    }
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


//will set up this view to receive a cover from the given view
void TiltFlowView::setUpIncomingCover( TiltFlowView &view )
{
	mpCube->orientTo( *view.mpCube );
	//just to set it's mLastNeighboredSide
	getNeighbor();
	SetStatus( TiltFlowView::STATUS_STARTING );
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


