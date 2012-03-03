#include "config.h"

#include "InterstitialController.h"

namespace TotalsGame {

    InterstitialController::InterstitialController (Game *game) {
        mGame = game;
        iv = NULL;
    }

    void InterstitialController::OnSetup() {
        CORO_RESET;
        mDone = false;
        /* TODO
      mGame.CubeSet.NewCubeEvent += cube => new BlankView(cube);
      mGame.CubeSet.LostCubeEvent += delegate(Cube cube) {
        if (!mDone && cube.userData is InterstitialView) {
          mDone = true;
          mGame.sceneMgr.QueueTransition("Next");
        }
      };
      */
    }

    float InterstitialController::Coroutine() {

        const float kTransitionTime = 0.5f;
        static char ivBuffer[sizeof(InterstitialView)];
        static char blankViewBuffer[NUM_CUBES][sizeof(BlankView)];

        CORO_BEGIN;

        iv = new(ivBuffer) InterstitialView(Game::GetCube(0));
#if !DISABLE_CHAPTERS
        if (mGame->IsPlayingRandom()) {
#endif //!DISABLE_CHAPTERS
            iv->message = "Random!";
            iv->image = &Hint_6;
#if !DISABLE_CHAPTERS
        } else {
            iv->message = Database::NameOfChapter(mGame->currentPuzzle->chapterIndex);
            static const PinnedAssetImage *hints[] =
            {
                &Hint_0,&Hint_1,&Hint_2,&Hint_3,&Hint_4,&Hint_5,&Hint_6
            };
            iv->image = hints[mGame->currentPuzzle->chapterIndex];
        }
#endif //!DISABLE_CHAPTERS
        for(int i = 1; i < NUM_CUBES; i++)
        {
            new(blankViewBuffer[i]) BlankView(Game::cubes[0], NULL);
        }

        CORO_YIELD(0.333f);
        AudioPlayer::PlayShutterOpen();
        iv->TransitionSync(kTransitionTime, true);
        PLAY_SFX(sfx_Tutorial_Correct);
        CORO_YIELD(3);
        AudioPlayer::PlayShutterClose();
        iv->TransitionSync(kTransitionTime, false);
        CORO_YIELD(0.333f);

        CORO_END;
        return -1;
    }

    void InterstitialController::OnTick () {        

        UPDATE_CORO(Coroutine);
            mGame->sceneMgr.QueueTransition("Next");
    }

    void InterstitialController::OnPaint (bool canvasDirty) {
        if (canvasDirty) { Game::PaintCubeViews(); }
    }

    void InterstitialController::OnDispose () {
        Game::ClearCubeEventHandlers();
        Game::ClearCubeViews();
    }



}

