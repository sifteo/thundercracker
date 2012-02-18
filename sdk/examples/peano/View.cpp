#include "View.h"
#include "TotalsCube.h"

namespace TotalsGame
{
DEFINE_POOL(View)

View::View(TotalsCube *_cube)
{
    mCube = NULL;
    mLockCount = 0;
    SetCube(_cube);
    mCube->SetView(this);
}

void View::PaintViews(TotalsCube *cubes, int numCubes)
{
    for(int i = 0; i < numCubes; i++)
    {
        View *v = cubes[i].GetView();
        if(v) v->Paint();
    }
}

void View::SetCube(TotalsCube *c)
{
    if (mCube != c)
    {
        if (mCube != NULL)
        {
            WillDetachFromCube(mCube);
            //if (willDetachFromCube != null) { willDetachFromCube(this); }
            mCube->SetView(NULL);
        }
        mCube = c;
        if (mCube != NULL)
        {
            View *view = mCube->GetView();
            if (view != NULL)
            {
                view->SetCube(NULL);
            }
            c->SetView(this);
            DidAttachToCube(c);
            //if (didAttachToCube != null) { didAttachToCube(this); }
            Paint();
        }
    }

}

TotalsCube *View::GetCube()
{
    return mCube;
}

void View::Lock() {
    mLockCount++;
}

void View::Unlock() {
    if (IsLocked()) {
        mLockCount--;
        if (mLockCount == 0 /*&& mDirty*/) {
            //TODO Paint();
        }
    }
}

bool View::IsLocked()
{
    return mLockCount > 0;
}

bool View::OkayToPaint()
{
    return mCube != NULL && mLockCount == 0 /*&& mPaintOverride == null*/;
}


}
