#pragma  once

#include "sifteo.h"
#include "View.h"
#include "Game.h"
#include "ObjectPool.h"

namespace TotalsGame 
{

	class BlankView : public View 
	{
	public:

        BlankView();
		virtual ~BlankView() {}

        virtual void Paint();

        const Sifteo::PinnedAssetImage *assetImage;     
	};

}

