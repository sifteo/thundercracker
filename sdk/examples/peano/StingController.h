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
		class EventHandler: public TotalsCube::EventHandler
		{
			StingController *owner;
		public:
			EventHandler(StingController *s);
			virtual void OnCubeTouch(TotalsCube *cube, bool touching);
			virtual void OnCubeShake(TotalsCube *cube);
		};

		Game *mGame;

		CORO_PARAMS
		float time;
		int i;

		EventHandler eventHandler;

        float Coroutine(float dt);

	public:
		StingController(Game *game);

		void Skip();

		//IStateController
		virtual void OnSetup ();
        virtual void OnTick (float dt);
		virtual void OnPaint (bool canvasDirty);
		virtual void OnDispose ();

	};

}

