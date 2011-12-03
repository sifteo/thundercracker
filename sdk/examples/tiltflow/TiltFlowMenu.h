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
  //public readonly Color color;
    const char *mName;
    //char mDescription[32];
    const Sifteo::AssetImage &mImage;
    //Vec2 mSrcPos;
  //public readonly Vec2 sourcePosition;

  TiltFlowItem(const Sifteo::AssetImage &image, const char *pName/*, string description, Vec2 sourcePosition=new Vec2()*/);
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
  typedef enum
  { STATUS_NONE, STATUS_MENU, STATUS_ITEM, STATUS_INFO }
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
  inline void SetDirty() { mDirty = true; }
  inline void SetCube( Cube *pCube ) { mpCube = pCube; }

  void CheckForRepaint();
  void Tick();
  //void RelateToMenu();
private:
  Status mStatus;
  int mItem;
  float mOffsetX;
  float mAccel;
  float mRestTime;
  bool mDrawLabel;
  bool mDirty;
  Side mLastNeighborRemoveSide;
  Cube *mpCube;

  // for PositionOf and FindGroups
  //bool mVisited;
  //Vec2 mGridPosition;




  void PaintNone();
  void PaintInfo();
  void PaintMenu();

  void DoPaintItem(TiltFlowItem *pItem, int x, int w);
  void PaintItem();

  void OnButton(bool pressed);

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
    static const float REST_DELAY;

    typedef enum
    { CHOOSING, PICKED }
    Status;

    static TiltFlowMenu *Inst();

    TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes);

    void AssignViews();

    void Tick(float dt);
    void Pick(TiltFlowView &view);

    inline TiltFlowView *GetKeyView() { return mKeyView; }
    inline float GetSimTime() const { return mSimTime; }
    inline Status GetStatus() const { return mStatus; }
    inline int GetNumItems() const { return mNumItems; }
    TiltFlowItem *GetItem( int item );

private:
    //void CheckMenuNeighbors();
    //void ReassignMenu();

    static TiltFlowMenu *s_pInst;

    //user provided!
    TiltFlowItem *mItems;
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
