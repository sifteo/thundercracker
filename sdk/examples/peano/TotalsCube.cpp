#include "TotalsCube.h"
#include "assets.gen.h"
#include "Game.h"

namespace TotalsGame
{
	TotalsCube::TotalsCube(): Sifteo::Cube()
	{
		CORO_RESET;
		shuttersOpen = false;
		shuttersClosed = false;
	}

	void TotalsCube::OpenShutters(const char *image)
	{						
		CORO_BEGIN

			shuttersOpen = false;

			//TODO Jukebox.PlayShutterOpen();
			for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
			{
				DrawVaultDoorsOpenStep1(32.0f * t/kTransitionTime, image);
				//Game::Yield();
				CORO_YIELD;
			}

			DrawVaultDoorsOpenStep1(32, image);			
			//Game::Yield();
			CORO_YIELD;

			for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt)
			{
				DrawVaultDoorsOpenStep2(32.0f * t/kTransitionTime, image);
				//Game::Yield();
				CORO_YIELD;
			}
			CORO_END

				shuttersOpen = true;

				CORO_RESET;
	}

	void TotalsCube::CloseShutters()
	{
		CORO_BEGIN

			shuttersClosed = false;

			//TODO Jukebox.PlayShutterClose();
			for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
			{
				DrawVaultDoorsOpenStep2(32.0f - 32.0f * t/kTransitionTime, "");
				CORO_YIELD;
			}

			DrawVaultDoorsOpenStep2(0, "");
			CORO_YIELD;

			for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
			{
				DrawVaultDoorsOpenStep1(32.0f - 32.0f * t/kTransitionTime, "");				
				CORO_YIELD;
			}			

			CORO_END

			shuttersClosed = true;

			CORO_RESET;
	}

	bool TotalsCube::AreShuttersOpen()
	{
		return shuttersOpen;
	}

	bool TotalsCube::AreShuttersClosed()
	{
		return shuttersClosed;
	}

	void TotalsCube::Image(AssetImage *image)
	{
		VidMode_BG0 mode(vbuf);
		mode.init();
		mode.BG0_drawAsset(Vec2(0,0), *image, 0);
	}


	// for these methods, 0 <= offset <= 32

	void TotalsCube::DrawVaultDoorsClosed()
	{
		VidMode_BG0 mode(vbuf);
		mode.init();

		mode.BG0_drawPartialAsset(Vec2(0,0),Vec2(9,9),Vec2(7,7), VaultDoor, 0);
		mode.BG0_drawPartialAsset(Vec2(7,0),Vec2(0,9),Vec2(9,7), VaultDoor, 0);
		mode.BG0_drawPartialAsset(Vec2(0,7),Vec2(9,0),Vec2(7,9), VaultDoor, 0);
		mode.BG0_drawPartialAsset(Vec2(7,7),Vec2(0,0),Vec2(9,9), VaultDoor, 0);
	}

	void TotalsCube::DrawVaultDoorsOpenStep1(int offset, const char*) 
	{
		VidMode_BG0 mode(vbuf);
		mode.init();

		int yTop = /*TokenView.Mid.y*/7 - (offset+4)/8;
		int yBottom = /*TokenView.Mid.y*/7 + (offset+4)/8;

		mode.BG0_drawPartialAsset(Vec2(0,0), Vec2(9,16-yTop), Vec2(7,yTop), VaultDoor);			//Top left
		mode.BG0_drawPartialAsset(Vec2(7,0), Vec2(0,16-yTop), Vec2(9,yTop), VaultDoor);			//top right
		mode.BG0_drawPartialAsset(Vec2(0,yBottom), Vec2(9,0), Vec2(7,16-yBottom), VaultDoor);	//bottom left
		mode.BG0_drawPartialAsset(Vec2(7,yBottom), Vec2(0,0), Vec2(9,16-yBottom), VaultDoor);	//bottom right

		//TODO
		//if (innerImage.Length > 0 && offset > 0) {
		//	cube.Image(innerImage, 0, yTop, 0, yTop, 8, yBottom-yTop); // "inner" row
		//} 
	}

	void TotalsCube::DrawVaultDoorsOpenStep2(int offset, const char*) 
	{
		VidMode_BG0 mode(vbuf);
		mode.init();

		int xLeft = /*TokenView.Mid.x*/7 - (offset+4)/8;
		int xRight = /*TokenView.Mid.x*/7 + (offset+4)/8;

		mode.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-xLeft,16-(7-4)), Vec2(xLeft,7-4), VaultDoor);			//Top left
		mode.BG0_drawPartialAsset(Vec2(xRight,0), Vec2(0,16-(7-4)), Vec2(16-xRight,7-4), VaultDoor);		//Top right
		mode.BG0_drawPartialAsset(Vec2(0,7+4), Vec2(16-xLeft,0), Vec2(xLeft,16-(7+4)), VaultDoor);			//bottom left
		mode.BG0_drawPartialAsset(Vec2(xRight,7+4), Vec2(0,0), Vec2(16-xRight,16-(7+4)), VaultDoor);			//bottom right

		//TODO
		//if (innerImage.Length > 0 && offset > 0) {
		//	cube.Image(innerImage, xLeft, 0, xLeft, 0, xRight-xLeft, 128); // "inner" column
		//}
	}

}