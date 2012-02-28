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

		BlankView(TotalsCube *c, const Sifteo::AssetImage *image);

		virtual ~BlankView() {}

		void SetImage(const Sifteo::AssetImage *image, bool andPaint=true);

		bool HasImage();

		virtual void Paint();
		virtual void Update (float dt);

        //for placement new
        void* operator new (size_t size, void* ptr) throw() {return ptr;}
        void operator delete(void *ptr) {}
	
	private:		
		const Sifteo::AssetImage *assetImage;

	};

}

