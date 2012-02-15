#include "StingController.h"

namespace TotalsGame 
{

	StingController::EventHandler::EventHandler(StingController *s) 
	{
		owner = s;
	}

	void StingController::EventHandler::OnCubeTouch(TotalsCube *cube, bool touching) 
	{
		if(!touching)
			owner->Skip();
	}

	void StingController::EventHandler::OnCubeShake(TotalsCube *cube) 
	{
		owner->Skip();
	}

	StingController::StingController(Game *game): 
	eventHandler(this)
	{
		mGame = game;
	}

	//IStateController

	void StingController::OnSetup () 
	{
		CORO_RESET;

		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++) 
		{
			new BlankView(&Game::GetInstance().cubes[i], NULL);
            Game::GetCube(i)->AddEventHandler(&eventHandler);
		}

		//TODO
		//mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
		//mGame.CubeSet.NewCubeEvent += delegate { Skip(); };
	}

	void StingController::Skip() 
	{
		mGame->sceneMgr.QueueTransition("Next");
	}

	float StingController::OnTick (float dt) 
	{               
		CORO_BEGIN

			CORO_YIELD(0.1f);
		AudioPlayer::PlaySfx(sfx_Stinger_02);

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

	void StingController::OnPaint (bool canvasDirty)
	{
		if (canvasDirty) { View::PaintViews(Game::GetInstance().cubes, Game::NUMBER_OF_CUBES); }
	}

	void StingController::OnDispose () 
	{
		//TODO purge pending events?
		Game::ClearCubeEventHandlers();
		Game::ClearCubeViews();

	}

}

