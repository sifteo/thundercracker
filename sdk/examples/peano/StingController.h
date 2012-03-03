#include "StateMachine.h"
#include "View.h"
#include "BlankView.h"
#include "coroutine.h"
#include "assets.gen.h"
#include "AudioPlayer.h"
#include "Game.h"

namespace TotalsGame 
{

	namespace StingController
	{

		class EventHandler: public TotalsCube::EventHandler
		{			
		public:
			virtual void OnCubeTouch(TotalsCube *cube, bool touching);
			virtual void OnCubeShake(TotalsCube *cube);
		};

		float time;
		int i;

        EventHandler eventHandlers[NUM_CUBES];

        Game::GameState Coroutine();

		void Skip();


	};

}

