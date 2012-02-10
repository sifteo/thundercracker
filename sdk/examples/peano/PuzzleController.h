#pragma once

#include "StateMachine.h"
#include "PuzzleHelper.h"
#include "AudioPlayer.h"
#include "TokenView.h"

#include <string.h>

namespace TotalsGame {
	class PuzzleController: public IStateController 
	{
	private:
		CORO_PARAMS;
		int static_i;

		class EventHandler: public TotalsCube::EventHandler
		{
			void OnCubeShake(TotalsCube *cube) {}
			void OnCubeTouch(TotalsCube *cube, bool touching) {}
		};
		EventHandler eventHandler;

	public:
		//-------------------------------------------------------------------------
		// PARAMETERS
		//-------------------------------------------------------------------------

		Game *game;
		Puzzle *puzzle;
		bool mHasTransitioned;
		bool mPaused;

		bool IsPaused() 
		{
			return mPaused || game->IsPaused;      
		}

		//-------------------------------------------------------------------------
		// SETUP
		//-------------------------------------------------------------------------

		PuzzleController(Game *_game) { game = _game; mPaused = false;}

		virtual void OnSetup () 
		{
			CORO_RESET;

			AudioPlayer::PlayInGameMusic();
			mHasTransitioned = false;
			char stringRep[10];
			int stringRepLength;
			if (game->IsPlayingRandom()) 
			{
				int nCubes = 3 + ( Game::NUMBER_OF_CUBES > 3 ? Random().randrange(Game::NUMBER_OF_CUBES-3+1) : 0 );
				puzzle = PuzzleHelper::SelectRandomTokens(game->difficulty, nCubes);
				// hacky reliability :P
				int retries = 0;
				do
				{
					if (retries == 100)
					{
						retries = 0;
						puzzle = PuzzleHelper::SelectRandomTokens(game->difficulty, nCubes);
					} 
					else 
					{
						retries++;
					}
					puzzle->SelectRandomTarget();
					puzzle->target->GetValue().ToString(stringRep, 10);
					stringRepLength = strlen(stringRep);					
				} while(stringRepLength > 4 || puzzle->target->GetValue().IsNan());
			} 
			else
			{
				puzzle = game->currentPuzzle;
				puzzle->ClearGroups();
				puzzle->target->RecomputeValue(); // in case the difficulty changed
			}
			mPaused = false;
			puzzle->hintsUsed = 0;			

			for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
			{
                Game::GetCube(i)->AddEventHandler(&eventHandler);
			}
			
			//TODO game.CubeSet.LostCubeEvent += OnCubeLost;
		}

		/* TODO
		on cube found put a blank view on it
		void OnCubeLost(Cube c) {
		if (!c.IsUnused()) {
		// for now, let's just return to the main menu
		Transition("Quit");
		}
		}
		*/
		//-------------------------------------------------------------------------
		// MAIN CORO
		//-------------------------------------------------------------------------

		float TheBigCoroutine(float dt)
		{

			CORO_BEGIN

			for(int i = puzzle->GetNumTokens(); i < Game::NUMBER_OF_CUBES; i++)
			{
				new BlankView(Game::GetCube(i), NULL);
			}
			Game::DrawVaultDoorsClosed();
			CORO_YIELD(0.25);

			for(static_i = 0; static_i < puzzle->GetNumTokens(); static_i++)
			{
				float dt;
				while((dt = Game::GetCube(static_i)->OpenShutters(&Background)) >= 0)
				{
					CORO_YIELD(dt);
				}

				new TokenView(Game::GetCube(static_i), puzzle->GetToken(static_i), true);
				CORO_YIELD(0.1f);

			}


#if 0
			theRememberedi = 1;
			time = System::clock();
			while(static_i<=puzzle->GetNumTokens())
			{
				for(int i = 0; i < i; i++)
				{

						game->cubes[i].OpenShutters(&Background);
				}

				//new TokenView(puzzle.tokens[i], true).Cube = game.CubeSet[i];
				if(System::clock() - time >= 1.1f)
				{
					theRememberedi++;
					time = System::clock();
				}
				//CORO_YIELD;
				System::paint();
			}



			{ // flourish in
				yield return 1.1f;
				// show total
				Jukebox.Sfx("PV_tutorial_mix_nums");
				foreach(var token in puzzle.tokens)
				{ 
					token.GetView().ShowOverlay(); 
				}
				yield return 3f;
				foreach(var token in puzzle.tokens) 
				{ 
					token.GetView().HideOverlay();
				}
			}

			{ // gameplay
				// subscribe to events
				game.CubeSet.NeighborAddEvent += OnNeighborAdd;
				game.CubeSet.NeighborRemoveEvent += OnNeighborRemove;
				{ // game loop
					var pauseHelper = new PauseHelper();
					while(!puzzle.IsComplete) {
						// most stuff is handled by events and view updaters
						yield return 0f;
						// should pause?
						pauseHelper.Update();
						if (pauseHelper.IsTriggered) {
							mPaused = true;

							// transition out
							for(var i=0; i<puzzle.tokens.Length; ++i) {
								foreach(var dt in game.CubeSet[i].CloseShutters()) { yield return dt; }
								new BlankView(game.CubeSet[i]);
							}

							// do menu
							using (var menu = new ConfirmationMenu("Return to Menu?", game)) {
								while(!menu.IsDone) {
									yield return 0;
									menu.Tick(game.dt);
								}
								if (menu.Result) {
									game.sceneMgr.QueueTransition("Quit");
									yield break;
								}
							}

							// transition back
							for(int i=0; i<puzzle.tokens.Length; ++i) {
								foreach (var dt in game.CubeSet[i].OpenShutters("background")) { yield return dt; }
								puzzle.tokens[i].GetView().Cube = game.CubeSet[i];
								yield return 0.1f;
							}

							pauseHelper.Reset();
							mPaused = false;
						}
					}
				}
				// unsubscribe
				game.CubeSet.NeighborAddEvent -= OnNeighborAdd;
				game.CubeSet.NeighborRemoveEvent -= OnNeighborRemove;
			}

			{ // flourish out
				yield return 1f;
				Jukebox.Sfx("PV_level_clear");
				foreach(var token in puzzle.tokens) { token.GetView().ShowLit(); }
				yield return 3f;
			}

			{ // transition out
				for(int i=0; i<puzzle.tokens.Length; ++i) {
					foreach(var dt in game.CubeSet[i].CloseShutters()) { yield return dt; }
					new BlankView(game.CubeSet[i]);
					yield return 0.1f;
				}
			}

			if (!game.PlayingRandom) { // closing remarks
				int count = puzzle.CountAfterThisInChapterWithCurrentCubeSet();
				if (count > 0) {
					var nv = new NarratorView();
					nv.Cube = game.CubeSet[0];
					const float kTransitionTime = 0.2f;
					Jukebox.PlayShutterOpen();
					for(var t=0f; t<kTransitionTime; t+=game.dt) {
						nv.SetTransitionAmount(t/kTransitionTime);
						yield return 0;
					}
					nv.SetTransitionAmount(1f);
					yield return 0.5f;
					if (count == 1) {
						nv.SetMessage("1 code to go...", "mix01");
						int i=0;
						yield return 0;
						for(var t=0f; t<3f; t+=game.dt) {
							i = 1-i;
							nv.SetEmote("mix0"+(i+1));
							yield return 0;
						}
					} else {
						nv.SetMessage(string.Format("{0} codes to go...", count));
						yield return 2f;
					}
					Jukebox.PlayShutterClose();
					for(var t=0f; t<kTransitionTime; t+=game.dt) {
						nv.SetTransitionAmount(1f-t/kTransitionTime);
						yield return 0;
					}
					nv.SetTransitionAmount(0f);
				}
			}

			Transition("Complete");
#endif

			CORO_END
			return -1;
		}

		//-------------------------------------------------------------------------
		// EVENTS
		//-------------------------------------------------------------------------
#if 0
		void OnNeighborAdd(Cube c, Cube.Side s, Cube nc, Cube.Side ns) {
			if (IsPaused || mRemoveEventBuffer.Count > 0) { return; }

			// validate args
			var v = c.GetTokenView();
			var nv = nc.GetTokenView();
			if (v == null || nv == null) { return; }
			if (s != CubeHelper.InvertSide(ns)) { return; }
			if (s == Cube.Side.LEFT || s == Cube.Side.TOP) {
				OnNeighborAdd(nc, ns, c, s);
				return;
			}
			var t = v.token;
			var nt = nv.token;
			// resolve solution
			if (t.current != nt.current) {
				Jukebox.PlayNeighborAdd();
				var grp = TokenGroup.Connect(t.current, t, Int2.Side(s), nt.current, nt);
				if (grp != null) {
					foreach(var token in grp.Tokens) { token.GetView().WillJoinGroup(); }
					grp.SetCurrent(grp);
					foreach(var token in grp.Tokens) { token.GetView().DidJoinGroup(); }
				}
			}
		}

		struct RemoveEvent {
			public Cube c;
			public Cube.Side s;
			public Cube nc;
			public Cube.Side ns;
		}
		List<RemoveEvent> mRemoveEventBuffer = new List<RemoveEvent>();

		void OnNeighborRemove(Cube c, Cube.Side s, Cube nc, Cube.Side ns) {
			if (IsPaused || mRemoveEventBuffer.Count > 0) {
				mRemoveEventBuffer.Add(new RemoveEvent() { c = c, s = s, nc = nc, ns = ns });
				return;
			}

			// validate args
			var v = c.GetTokenView();
			var nv = nc.GetTokenView();
			if (v == null || nv == null) { return; }
			var t = v.token;
			var nt = nv.token;
			// resolve soln
			if (t.current == null || nt.current == null || t.current != nt.current) {
				return;
			}
			Jukebox.PlayNeighborRemove();
			var grp = t.current as TokenGroup;
			while(t.current.Contains(nt)) {
				foreach(var token in puzzle.tokens) {
					if (token != t && token.current == t.current) {
						token.PopGroup();
					}
				}
				t.PopGroup();
			}
			foreach(var token in grp.Tokens) { token.GetView().DidGroupDisconnect(); }
		}

		void ProcessRemoveEventBuffer() {
			var args = mRemoveEventBuffer;
			mRemoveEventBuffer = new List<RemoveEvent>(args.Count);
			foreach(var arg in args) {
				OnNeighborRemove(arg.c, arg.s, arg.nc, arg.ns);
			}
		}
#endif
		//-------------------------------------------------------------------------
		// CLEANUP
		//-------------------------------------------------------------------------

		virtual float OnTick (float dt) {
			if (mHasTransitioned) { return -1; }
			/* TODO			if (!IsPaused && mRemoveEventBuffer.Count > 0) {
			ProcessRemoveEventBuffer();
			} */
			return TheBigCoroutine(dt);
			/*			if (mMainCoro.Update(dt)) {
			game.CubeSet.UpdateViews(dt);
			} */
		}

		virtual void OnPaint (bool canvasDirty)
		{
			//if (canvasDirty) { game.CubeSet.PaintViews(); }
		}

		void Transition(const char *id) {
			if (!mHasTransitioned) {
				mHasTransitioned = false;
				game->sceneMgr.QueueTransition(id);
			}
		}

		virtual void OnDispose () {
			//mRemoveEventBuffer.Clear();
			//puzzle.ClearUserdata();
			//puzzle = null;
			//mPaused = false;
            Game::ClearCubeEventHandlers();
            Game::ClearCubeViews();
		}
	};
}

