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
		BlankView(TotalsCube *c, const Sifteo::AssetImage *image);
		virtual ~BlankView() {}

        virtual void Paint();

        const Sifteo::AssetImage *assetImage;

        //for placement new
        void* operator new (size_t size, void* ptr) throw() {return ptr;}
        void operator delete(void *ptr) {}
	
	private:		


	};

}

