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
		BlankView *views[Game::NUMBER_OF_CUBES];

		CORO_PARAMS
		float time;

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
				views[i] = new BlankView(&Game::GetInstance().cubes[i], NULL);
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

		void OnTick (float dt) 
		{
			CORO_BEGIN

			CORO_WAIT(0.1f);
			AudioPlayer::PlaySfx(sfx_Stinger2);

			{
				bool anyOpen = false;
				for(int i = 0; i < Game::NUMBER_OF_CUBES; i++) 
				{
					views[i]->cube->OpenShutters(&Title);
					anyOpen |= views[i]->cube->AreShuttersOpen();
				}
				if(!anyOpen)
					return;
			}

			CORO_WAIT(3.0);

			{
				bool anyClosed = false;
				for(int i = 0; i < Game::NUMBER_OF_CUBES; i++) 
				{
					views[i]->cube->CloseShutters(&Title);
					anyClosed |= views[i]->cube->AreShuttersClosed();
				}				
				if(!anyClosed)
					return;
			}
			
			CORO_WAIT(0.5f);
			//yield return 0.5f;
			mGame->sceneMgr.QueueTransition("Next");

			CORO_END
		}

		void OnPaint (bool canvasDirty)
		{
		//	if (canvasDirty) { mGame.CubeSet.PaintViews(); }
		}

		void OnDispose () 
		{
			for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
			{
				delete views[i];
				views[i] = NULL;
			}
		/*	//TODO
			mGame.CubeSet.ClearEvents();
			mGame.CubeSet.ClearUserData(); */
		}

	};

}

