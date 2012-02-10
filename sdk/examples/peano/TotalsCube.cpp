#include "TotalsCube.h"
#include "assets.gen.h"
#include "Game.h"
#include "AudioPlayer.h"
#include "TokenView.h"

namespace TotalsGame
{
	TotalsCube::TotalsCube(): Sifteo::Cube()
	{
		CORO_RESET;
		view = NULL;
		eventHandler = NULL;
	}

    void TotalsCube::AddEventHandler(EventHandler *e)
    {
        e->next = eventHandler;
        e->prev = NULL;
        if(e->next)
        {
            e->next->prev = e;
        }
        eventHandler = e;
    }

    void TotalsCube::RemoveEventHandler(EventHandler *e)
    {
        if(e->prev)
        {
            e->prev->next = e->next;
        }
        else
        {
            assert(e == eventHandler);
            eventHandler = e->next;
        }

        if(e->next)
        {
            e->next->prev = e->prev;
        }
        e->next = e->prev = NULL;
    }

    void TotalsCube::ResetEventHandlers()
    {
        while(eventHandler)
        {
            RemoveEventHandler(eventHandler);
        }
    }

    void TotalsCube::DispatchOnCubeShake(TotalsCube *c)
    {
        EventHandler *e = eventHandler;
        while(e)
            e->OnCubeShake(c);
    }

    void TotalsCube::DispatchOnCubeTouch(TotalsCube *c, bool touching)
    {
        EventHandler *e = eventHandler;
        while(e)
            e->OnCubeTouch(c, touching);
    }


	float TotalsCube::OpenShutters(const AssetImage *image)
	{						
		CORO_BEGIN

		AudioPlayer::PlayShutterOpen();
		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
		{
			DrawVaultDoorsOpenStep1(32.0f * t/kTransitionTime, image);
			CORO_YIELD(0);
		}

		DrawVaultDoorsOpenStep1(32, image);			
		CORO_YIELD(0);

		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt)
		{
			DrawVaultDoorsOpenStep2(32.0f * t/kTransitionTime, image);
			CORO_YIELD(0);
		}
		CORO_END
		CORO_RESET;

		return -1;
	}

	float TotalsCube::CloseShutters(const AssetImage *image)
	{
		CORO_BEGIN

		AudioPlayer::PlayShutterClose();
		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
		{
			DrawVaultDoorsOpenStep2(32.0f - 32.0f * t/kTransitionTime, image);
			CORO_YIELD(0);
		}

		DrawVaultDoorsOpenStep2(0, image);
		CORO_YIELD(0);

		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
		{
			DrawVaultDoorsOpenStep1(32.0f - 32.0f * t/kTransitionTime, image);				
			CORO_YIELD(0);
		}			

		CORO_END
		CORO_RESET;

		return -1;
	}
	
	void TotalsCube::Image(const AssetImage *image)
	{
		VidMode_BG0 mode(vbuf);
		mode.init();
		mode.BG0_drawAsset(Vec2(0,0), *image, 0);
	}

	void TotalsCube::Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size)
	{
		VidMode_BG0 mode(vbuf);
		mode.init();
		mode.BG0_drawPartialAsset(coord, offset, size, *image, 0);
	}

	// for these methods, 0 <= offset <= 32

	void TotalsCube::DrawVaultDoorsClosed()
	{
		VidMode_BG0 mode(vbuf);
		mode.init();

		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		mode.BG0_drawPartialAsset(Vec2(0,0),Vec2(9,9),Vec2(x,y), VaultDoor, 0);
		mode.BG0_drawPartialAsset(Vec2(x,0),Vec2(0,9),Vec2(16-x,y), VaultDoor, 0);
		mode.BG0_drawPartialAsset(Vec2(0,y),Vec2(9,0),Vec2(x,16-y), VaultDoor, 0);
		mode.BG0_drawPartialAsset(Vec2(x,y),Vec2(0,0),Vec2(16-x,16-y), VaultDoor, 0);
	}

	void TotalsCube::DrawVaultDoorsOpenStep1(int offset, const AssetImage *innerImage) 
	{
		VidMode_BG0 mode(vbuf);
		mode.init();

		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int yTop = x - (offset+4)/8;
		int yBottom = y + (offset+4)/8;

		if(innerImage)
			mode.BG0_drawAsset(Vec2(0,0), *innerImage);

		mode.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-x,16-yTop), Vec2(x,yTop), VaultDoor);			//Top left
		mode.BG0_drawPartialAsset(Vec2(x,0), Vec2(0,16-yTop), Vec2(16-x,yTop), VaultDoor);			//top right
		mode.BG0_drawPartialAsset(Vec2(0,yBottom), Vec2(16-x,0), Vec2(x,16-yBottom), VaultDoor);	//bottom left
		mode.BG0_drawPartialAsset(Vec2(x,yBottom), Vec2(0,0), Vec2(16-x,16-yBottom), VaultDoor);	//bottom right

		/*
		if (innerImage && yTop != yBottom) 
		{
			mode.BG0_drawPartialAsset(Vec2(0,yTop), Vec2(0,yTop), Vec2(16,yBottom-yTop), *innerImage); // "inner" row
		} 
		*/
	}

	void TotalsCube::DrawVaultDoorsOpenStep2(int offset, const AssetImage *innerImage) 
	{
		VidMode_BG0 mode(vbuf);
		mode.init();

		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int xLeft = x - (offset+4)/8;
		int xRight = y + (offset+4)/8;

		if(innerImage)
			mode.BG0_drawAsset(Vec2(0,0), *innerImage);

		mode.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-xLeft,16-(y-4)), Vec2(xLeft,y-4), VaultDoor);			//Top left
		mode.BG0_drawPartialAsset(Vec2(xRight,0), Vec2(0,16-(y-4)), Vec2(16-xRight,y-4), VaultDoor);		//Top right
		mode.BG0_drawPartialAsset(Vec2(0,y+4), Vec2(16-xLeft,0), Vec2(xLeft,16-(y+4)), VaultDoor);			//bottom left
		mode.BG0_drawPartialAsset(Vec2(xRight,y+4), Vec2(0,0), Vec2(16-xRight,16-(y+4)), VaultDoor);			//bottom right
		
		/*
		if (innerImage && xLeft != xRight) {
			mode.BG0_drawPartialAsset(Vec2(xLeft,0), Vec2(xLeft,0), Vec2(xRight-xLeft,16), *innerImage); // "inner" column
		}
		*/
	}

}
