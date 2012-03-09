#include "BlankView.h"

namespace TotalsGame 
{

BlankView::BlankView()
{
    assetImage = NULL;
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

}

