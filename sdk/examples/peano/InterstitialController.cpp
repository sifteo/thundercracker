#include "config.h"

#include "InterstitialController.h"

namespace TotalsGame {

namespace InterstitialController
{




    /* TODO lost and found
      mGame.CubeSet.NewCubeEvent += cube => new BlankView(cube);
      mGame.CubeSet.LostCubeEvent += delegate(Cube cube) {
        if (!mDone && cube.userData is InterstitialView) {
          mDone = true;
          mGame.sceneMgr.QueueTransition("Next");
        }
      };
      */



Game::GameState Run() {

    const float kTransitionTime = 0.5f;

    InterstitialView iv;
    Game::cubes[0].SetView(&iv);

#if !DISABLE_CHAPTERS
    if (Game::IsPlayingRandom()) {
#endif //!DISABLE_CHAPTERS
        iv.message = "Random!";
        iv.image = &Hint_6;
#if !DISABLE_CHAPTERS
    } else {
        iv.message = Database::NameOfChapter(Game::currentPuzzle->chapterIndex);
        static const PinnedAssetImage *hints[] =
        {
            &Hint_0,&Hint_1,&Hint_2,&Hint_3,&Hint_4,&Hint_5,&Hint_6
        };
        iv.image = hints[Game::currentPuzzle->chapterIndex];
    }
#endif //!DISABLE_CHAPTERS
    BlankView blankViews[NUM_CUBES];
    for(int i = 1; i < NUM_CUBES; i++)
    {
        Game::cubes[i].SetView(blankViews + i);
    }

    Game::Wait(0.333f);
    AudioPlayer::PlayShutterOpen();
    iv.TransitionSync(kTransitionTime, true);
    PLAY_SFX(sfx_Tutorial_Correct);
    Game::Wait(3);
    AudioPlayer::PlayShutterClose();
    iv.TransitionSync(kTransitionTime, false);
    Game::Wait(0.333f);

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return Game::GameState_Puzzle;
}





}

}

