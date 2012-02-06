#include "StateMachine.h"
#include "View.h"
#include "BlankView.h"
#include "coroutine.h"

namespace TotalsGame 
{

	class StingController : public IStateController 
	{
	private:
		Game *mGame;
		BlankView *views[Game::NUMBER_OF_CUBES];

		CORO_PARAMS

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

			//yield return 0.1f;
			CORO_YIELD;
			//TODO Jukebox.Sfx("PV_stinger_02");

			{
				bool anyOpen = false;
				for(int i = 0; i < Game::NUMBER_OF_CUBES; i++) 
				{
					views[i]->cube->OpenShutters(NULL/*TOTO title*/);
					anyOpen |= views[i]->cube->AreShuttersOpen();
				}
				if(!anyOpen)
					return;
			}

			CORO_YIELD;
			//yield return 3f;

			{
				bool anyClosed = false;
				for(int i = 0; i < Game::NUMBER_OF_CUBES; i++) 
				{
					views[i]->cube->CloseShutters();
					anyClosed |= views[i]->cube->AreShuttersClosed();
				}				
				if(!anyClosed)
					return;
			}
			
			CORO_YIELD;
			//yield return 0.5f;
			printf("NEXT!\n");
			mGame->sceneMgr.QueueTransition("Next");

			CORO_END


/*			if (mCoro != null) {
				mCoro.Update(dt);
				if (mCoro.IsDone) {
					mCoro = null;
					mGame.sceneMgr.QueueTransition("Next");
				} else {
					mGame.CubeSet.UpdateViews(Mathf.Min(dt, 0.2f));
				}
			}
			*/
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

