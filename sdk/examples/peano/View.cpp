#include "View.h"
#include "TotalsCube.h"

namespace TotalsGame
{
	DEFINE_POOL(View)

	View::View(TotalsCube *_cube)
	{
		cube = _cube;	
		cube->SetView(this);
	}

	void View::PaintViews(TotalsCube *cubes, int numCubes)
	{
		for(int i = 0; i < numCubes; i++)
		{
			View *v = cubes[i].GetView();
			if(v) v->Paint();
		}
	}
}