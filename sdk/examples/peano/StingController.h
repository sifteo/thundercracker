#include "StateMachine.h"
#include "View.h"
#include "BlankView.h"
#include "coroutine.h"
#include "assets.gen.h"
#include "AudioPlayer.h"

namespace TotalsGame 
{

	class StingController : public IStateController 
	{
	private:
		Game *mGame;

		CORO_PARAMS
		float time;
		int i;

	public:
		StingController(Game *game) 
		{
			mGame = game;
		}

		//IStateController

		virtual void OnSetup () 
		{
			CORO_RESET;

			for(int i = 0; i < Game::NUMBER_OF_CUBES; i++) 
			{
				new BlankView(&Game::GetInstance().cubes[i], NULL);
				//TODO
				//cube.ShakeStoppedEvent += delegate { Skip(); };
				//cube.ButtonEvent += delegate(Cube c, bool pressed) { if (!pressed) Skip();  };
			}

			//mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
			//mGame.CubeSet.NewCubeEvent += delegate { Skip(); };
		}

		void Skip() 
	{
			//TODO mGame.CubeSet.ClearEvents();
			mGame->sceneMgr.QueueTransition("Next");
		}

		float OnTick (float dt) 
		{
			CORO_BEGIN

			CORO_YIELD(0.1f);
			AudioPlayer::PlaySfx(sfx_Stinger2);

			for(i = 0; i < Game::NUMBER_OF_CUBES; i++) 
			{
				float dt;
				while((dt=Game::GetCube(i)->OpenShutters(&Title)) >= 0) 
				{
					CORO_YIELD(dt);
				}
				((BlankView*)Game::GetCube(i)->GetView())->SetImage(&Title);
				CORO_YIELD(0);
			}

			CORO_YIELD(3.0);

			for(i = 0; i < Game::NUMBER_OF_CUBES; i++) 
			{
				float dt;
				while((dt=Game::GetCube(i)->CloseShutters(&Title)) >= 0) 
				{
					CORO_YIELD(dt);
				}
				((BlankView*)Game::GetCube(i)->GetView())->SetImage(NULL);
				CORO_YIELD(0);
			}			
			
			CORO_YIELD(0.5f);

			mGame->sceneMgr.QueueTransition("Next");

			CORO_END

			return -1;
		}

		void OnPaint (bool canvasDirty)
		{
			if (canvasDirty) { View::PaintViews(Game::GetInstance().cubes, Game::NUMBER_OF_CUBES); }
		}

		void OnDispose () 
		{
		/*	//TODO
			mGame.CubeSet.ClearEvents(); */

			Game::ClearCubeViews();
			
		}

	};

}

