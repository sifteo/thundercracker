#pragma once

#include "sifteo.h"
#include "ObjectPool.h"
#include <stddef.h>

namespace TotalsGame
{
	class TotalsCube;

	class View
	{	
		TotalsCube *mCube;

        //only cube.setview is allowed to change my cube pointer
        friend class TotalsCube;

	public:
        View();

		virtual void Paint() {}
		virtual void DidAttachToCube(TotalsCube *c) {}
		virtual void WillDetachFromCube(TotalsCube *c) {}
        virtual ~View();

		TotalsCube *GetCube();

	};

}
