#include "View.h"
#include "TotalsCube.h"

namespace TotalsGame
{

View::View()
{
    mCube = NULL;
}

View::~View()
{
    if(mCube && mCube->GetView() == this)
    {
        mCube->SetView(NULL);
    }
}


TotalsCube *View::GetCube()
{
    return mCube;
}

}
