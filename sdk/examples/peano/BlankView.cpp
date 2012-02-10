#include "BlankView.h"

namespace TotalsGame 
{

	DEFINE_POOL(BlankView)


	BlankView::BlankView(TotalsCube *c, const Sifteo::AssetImage *image) : View(c)
	{
		assetImage = image;
	}

	void BlankView::SetImage(const Sifteo::AssetImage *image, bool andPaint) 
	{
		if (assetImage != image) 
		{
			assetImage = image;
			if (andPaint) 
			{ 
				Paint(); 
			}
		}
	}

	bool BlankView::HasImage()
	{
		return assetImage != NULL;
	}

	void BlankView::Paint() 
	{
		if (assetImage) 
		{
			GetCube()->Image(assetImage);
		} 
		else 
		{
			GetCube()->DrawVaultDoorsClosed();
		}
	}

	void BlankView::Update (float dt) 
	{
	}


}

