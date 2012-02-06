#pragma once

#include "assets.gen.h"
#include "coroutine.h"

namespace TotalsGame
{
	class View
	{
		CORO_PARAMS
		float t;

	public:
		View(Sifteo::Cube *_cube)
		{
			cube = _cube;
			CORO_RESET;
		}

		static const float kTransitionTime = 1.1f;

		void OpenShutters(const char *image)
		{						
			CORO_BEGIN

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

			CORO_RESET;
		}

		void CloseShutters()
		{
			CORO_BEGIN

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
			CORO_RESET;
		}



		// for these methods, 0 <= offset <= 32
/*
		static void DrawVaultDoorsClosed(Cube *cubes, int nCubes) 
		{
			for(int i=0; i<nCubes; ++i) {
				cubes[i].DrawVaultDoorsClosed();
			}
		}
		*/
		void DrawVaultDoorsClosed()
		{
			VidMode_BG0 mode(cube->vbuf);
			mode.init();
			
			mode.BG0_drawPartialAsset(Vec2(0,0),Vec2(9,9),Vec2(7,7), VaultDoor, 0);
			mode.BG0_drawPartialAsset(Vec2(7,0),Vec2(0,9),Vec2(9,7), VaultDoor, 0);
			mode.BG0_drawPartialAsset(Vec2(0,7),Vec2(9,0),Vec2(7,9), VaultDoor, 0);
			mode.BG0_drawPartialAsset(Vec2(7,7),Vec2(0,0),Vec2(9,9), VaultDoor, 0);
		}

		void DrawVaultDoorsOpenStep1(int offset, const char*) 
		{
			VidMode_BG0 mode(cube->vbuf);
			mode.init();

			int yTop = /*TokenView.Mid.y*/7 - (offset+4)/8;
			int yBottom = /*TokenView.Mid.y*/7 + (offset+4)/8;

			//mode.BG0_drawPartialAsset(Vec2(7+16-8,16+yTop-8), Vec2(0,0), Vec2(16,16), VaultDoor);
			int g = offset / 2;
			mode.BG0_drawPartialAsset(Vec2(0+g,0), Vec2(0,0), Vec2(16-g,16), VaultDoor);

			//cube.Image("vault_door", TokenView.Mid.x-8, yTop-8);  // top left
			//cube.Image("vault_door", TokenView.Mid.x, yTop-8);      // top right
			//cube.Image("vault_door", TokenView.Mid.x-8, yBottom);   // bottom left
			//cube.Image("vault_door", TokenView.Mid.x, yBottom);       // bottom right
			//if (innerImage.Length > 0 && offset > 0) {
			//	cube.Image(innerImage, 0, yTop, 0, yTop, 8, yBottom-yTop); // "inner" row
			//} 
		}

		void DrawVaultDoorsOpenStep2(int offset, const char*) 
		{/*
			int xLeft = TokenView.Mid.x - offset;
			int xRight = TokenView.Mid.x + offset;
			cube.Image("vault_door", xLeft-128, TokenView.Mid.y-32-128);  // top left
			cube.Image("vault_door", xRight, TokenView.Mid.y-32-128);     // top right
			cube.Image("vault_door", xLeft-128, TokenView.Mid.y+32);      // bottom left
			cube.Image("vault_door", xRight, TokenView.Mid.y+32);         // bottom right
			if (innerImage.Length > 0 && offset > 0) {
				cube.Image(innerImage, xLeft, 0, xLeft, 0, xRight-xLeft, 128); // "inner" column
			}*/
		}
		


	private:
		Sifteo::Cube *cube;
	};

}