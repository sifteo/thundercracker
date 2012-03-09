#include "BlankView.h"

namespace TotalsGame 
{

BlankView::BlankView(): View(NULL)
{
    assetImage = NULL;
}

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
            GetCube()->Image(assetImage, Vec2(0,0));
		} 
		else 
		{
			GetCube()->DrawVaultDoorsClosed();
		}
	}

	void BlankView::Update () 
	{
	}


}

