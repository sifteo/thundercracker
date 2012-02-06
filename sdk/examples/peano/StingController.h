#include "StateMachine.h"
#include "View.h"

namespace TotalsGame 
{

	class StingController : public IStateController 
	{
	private:
		Game *mGame;


	public:
		StingController(Game *game) 
		{
			mGame = game;
		}

		//IStateController

		virtual void OnSetup () 
		{
/*			foreach(Cube cube in mGame.CubeSet) {
				new BlankView(cube);
				cube.ShakeStoppedEvent += delegate { Skip(); };
				cube.ButtonEvent += delegate(Cube c, bool pressed) { if (!pressed) Skip();  };
			}
*/
			//mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
			//mGame.CubeSet.NewCubeEvent += delegate { Skip(); };
		}

/*		void Skip() {
			mGame.CubeSet.ClearEvents();
			mCoro = null;
			mGame.sceneMgr.QueueTransition("Next");
		}
*/
	/*	IEnumerator<float> Coroutine() {
			yield return 0.1f;
			Jukebox.Sfx("PV_stinger_02");
			foreach(var cube in mGame.CubeSet) {
				foreach(var dt in cube.OpenShutters("title")) { yield return dt; }
				var bv = cube.userData as BlankView;
				bv.SetImage("title");
				yield return 0f;
			}
			yield return 3f;
			foreach(var cube in mGame.CubeSet) {
				foreach(var dt in cube.CloseShutters()) { yield return dt; }
				var bv = cube.userData as BlankView;
				bv.SetImage("");
				yield return 0f;
			}
			yield return 0.5f;
		}
		*/
		void OnTick (float dt) 
		{
			static View v1(&Game::GetInstance().cubes[0]);
			static View v2(&Game::GetInstance().cubes[1]);
			static View v3(&Game::GetInstance().cubes[2]);
			static View v4(&Game::GetInstance().cubes[3]);

			v1.OpenShutters("");
			v2.OpenShutters("");
			v3.OpenShutters("");
			v4.OpenShutters("");





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
		/*	mCoro = null;
			mGame.CubeSet.ClearEvents();
			mGame.CubeSet.ClearUserData(); */
		}

	};

}

