  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

#ifndef _TILTFLOWMENU_H
#define _TILTFLOWMENU_H

#include <sifteo.h>
#include "utils.h"

using namespace Sifteo;

//---------------------------------------------------------------------------
// TILT FLOW ITEM
//---------------------------------------------------------------------------

class TiltFlowItem {
public:
    //const char *mName;
    const Sifteo::AssetImage &mImage;
    const Sifteo::AssetImage &mLabel;

  //TiltFlowItem(const Sifteo::AssetImage &image, const char *pName);
  TiltFlowItem(const Sifteo::AssetImage &image, const Sifteo::AssetImage &label);
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
  static const float EPSILON;
  static const float TIP_DELAY;

  static const int LABEL_OFFSET = 0;
  static const int TIP_Y_OFFSET = 12;
  static const int COVER_Y_OFFSET = -22;

  static const int NUM_TIPS = 2;
  static const Sifteo::AssetImage *TIPS[NUM_TIPS];

  typedef enum
  { 
	  STATUS_NONE, 
	  STATUS_MENU, 
	  STATUS_ITEM, 
	  STATUS_INFO, 
      //CES HACKERY
      STATUS_PICKED,
  }
  Status;

  static int s_cubeIndex;

  TiltFlowView();

  /*
  public int ItemIndex { get { return mItem; } }
  public TiltFlowItem Item { get { return menu[mItem]; } }*/

  //public readonly TiltFlowMenu menu;

  void Paint();

  inline Status GetStatus() const { return mStatus; }
  inline void SetStatus( Status status ) { mStatus = status; }
  inline void SetItem( int item ) { mItem = item; }
  inline int GetItem() const { return mItem; }
  inline void SetDirty() { mDirty = true; }
  inline void SetCube( Cube *pCube ) { mpCube = pCube; }
  inline bool IsFlushNeeded() const { return mFlushNeeded; }
  inline void Flush();

  void CheckForRepaint();
  void Tick();
  _SYSCubeID getNeighbor();
  void Cleanup();
  //void RelateToMenu();

private:
  Status mStatus;
  int mItem;
  float mOffsetX;
  float mAccel;
  float mRestTime;
  bool mDrawLabel;
  bool mDirty;
  unsigned int mCurrentTip;
  float mTipTime;
  Side mLastNeighborRemoveSide;
  Cube *mpCube;

  //for CES hackery
  Side mLastNeighboredSide;
  bool mFlushNeeded;

  // for PositionOf and FindGroups
  //bool mVisited;
  //Vec2 mGridPosition;

  void PaintNone();
  void PaintInfo();
  void PaintMenu();
  void PaintIncomingCover();

  void DoPaintItem(TiltFlowItem *pItem, int x, int y = 0);
  void PaintItem();

  void ClipIt(int ox, int &x, int &w);

  void OnButton(bool pressed);

  Vec2 LerpPosition( const Vec2 &start, const Vec2 &end, float timePercent );

  //TODO FLIP
  //void OnFlip(Cube c, bool newOrientationIsUp);

  //void OnNeighborAdd(Cube c, Side s, Cube nc, Side ns);
  //void OnNeighborRemove(Cube c, Side s, Cube nc, Side ns);

  void UpdateMenu();

  float Resistance();
  void CoastToStop();
  void StopScrolling();

  //used for neighboring, but not now
  /*
  bool PositionOf(Cube c, out Vec2 pos);
  bool PositionOfVisit(TiltFlowView origin, Cube target, Side s, out Vec2 result);
*/
};

struct TiltFlowItemArgs {
  TiltFlowView view;
  TiltFlowItem item;
  //AABB bounds;
};



class TiltFlowMenu
{
public:
    static const int MAX_CUBES = 32;

    static const float UPDATE_DELAY;
    static const float PICK_DELAY;
    static const float POST_PICK_DELAY;
    static const float REST_DELAY;

    typedef enum
    { 
		CHOOSING, 
		PICKED 
	}
    Status;

    static TiltFlowMenu *Inst();

    TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes);

    void Reset();
    void AssignViews();

    //return true unless done
	bool Tick(float dt);
    void Pick(TiltFlowView &view);

    inline TiltFlowView *GetKeyView() { return mKeyView; }
    inline float GetSimTime() const { return mSimTime; }
    inline Status GetStatus() const { return mStatus; }
    inline int GetNumItems() const { return mNumItems; }
	inline float GetScrollTime() const { return mSimTime - mPickTime; }
    TiltFlowItem *GetItem( int item );

    void playSound( _SYSAudioModule &sound );

	void showLogo();
    void checkNeighbors();

private:
    //void CheckMenuNeighbors();
    //void ReassignMenu();

    static TiltFlowMenu *s_pInst;

    //user provided!
    TiltFlowItem *mItems;
    Status mStatus;
    float mSimTime;
    float mUpdateTime;
    float mPickTime;
    int mNumCubes;
    int mNumItems;
    TiltFlowView *mKeyView;
    TiltFlowView mViews[ MAX_CUBES ];
	AudioChannel m_SFXChannel;
	bool mDone;
    bool mNeighborDirty;
};

/*
    public Action<TiltFlowView> PaintNone { internal get; set; }
    public Action<TiltFlowView> PaintBackground { internal get; set; }
    public Action<TiltFlowItemArgs> PaintItem { internal get; set; }

    public TiltFlowItem ResultItem { get { return mDone ? mKeyView.Item : NULL; } }

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



#endif //_TILTFLOWMENU_H
