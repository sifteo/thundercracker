#pragma  once

#include "sifteo.h"
#include "View.h"
#include "Game.h"
#include "ObjectPool.h"

namespace TotalsGame 
{

	class BlankView : public View 
	{
		DECLARE_POOL(BlankView, Game::NUMBER_OF_CUBES)

	public:

		BlankView(TotalsCube *c, const Sifteo::AssetImage *image);

		virtual ~BlankView() {}

		void SetImage(const Sifteo::AssetImage *image, bool andPaint=true);

		bool HasImage();

		virtual void Paint();
		virtual void Update (float dt);
	
	private:		
		const Sifteo::AssetImage *assetImage;

	};

}

