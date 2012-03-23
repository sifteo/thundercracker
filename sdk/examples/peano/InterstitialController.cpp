#include "InterstitialController.h"
#include "InterstitialView.h"
#include "AudioPlayer.h"
#include "Skins.h"

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

    Game::randomPuzzleCount = 0;
    Skins::SkinType skin = Skins::SkinType_Default;

    if (Game::IsPlayingRandom()) {
        iv.message = "Random!";
        iv.image = &Hint_6;
        skin = (Skins::SkinType)Game::rand.randrange(Skins::NumSkins);
    } else {
        iv.message = Database::NameOfChapter(Game::currentPuzzle->chapterIndex);
        static const PinnedAssetImage *hints[] =
        {
            &Hint_0,&Hint_1,&Hint_2,&Hint_3,&Hint_4,&Hint_5,&Hint_6
        };
        iv.image = hints[Game::currentPuzzle->chapterIndex];
        skin = (Skins::SkinType)(Game::currentPuzzle->chapterIndex % Skins::NumSkins);
    }

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].DrawVaultDoorsClosed();
    }
    System::paintSync();

    Skins::SetSkin(skin);

    Game::cubes[0].SetView(&iv);

    Game::Wait(0.333f);
    iv.TransitionSync(kTransitionTime, true);
    PLAY_SFX(sfx_Tutorial_Correct);
    Game::Wait(3);
    iv.TransitionSync(kTransitionTime, false);
    Game::Wait(0.333f);

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return Game::GameState_Puzzle;
}





}

}

