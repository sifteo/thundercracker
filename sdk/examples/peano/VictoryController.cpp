#include "VictoryController.h"
#include "NarratorView.h"
#include "VictoryView.h"
#include "BlankView.h"

namespace TotalsGame
{
namespace VictoryController
{
/* TODO lost and found
 mGame.CubeSet.NewCubeEvent += cb => new BlankView(cb);
 mGame.CubeSet.LostCubeEvent += OnLostCube;
 */

/*
void OnLostCube(Cube c) {
if (!c.IsUnused()) {
var blanky = mGame.CubeSet.FindAnyIdle();
if (blanky == null) {
Advance();
} else {
c.MoveViewTo(blanky);
}
}
}
*/

Game::GameState Run() {
#if !DISABLE_CHAPTERS
    NarratorView *nv;
    VictoryView *vv;

    bool isLast;

    static char nvBuffer[sizeof(NarratorView)];
    static char blankBuffers[NUM_CUBES][sizeof(BlankView)];
    static char vvBuffer[sizeof(VictoryView)];

    nv = new(nvBuffer) NarratorView(&Game::cubes[0]);

    for(int i=1; i<NUM_CUBES; ++i) {
        new(blankBuffers) BlankView(&Game::cubes[i], NULL);
    }
    static const float kTransitionTime = 0.2f;
    Game::Wait(0.5f);
    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        nv->SetTransitionAmount(t/kTransitionTime);
        nv->Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    nv->SetTransitionAmount(-1);
    Game::Wait(0.25f);
    nv->SetMessage("Codes accepted!\nGet ready!", NarratorView::EmoteYay);
    Game::Wait(3);
    nv->SetMessage("");

    AudioPlayer::PlayNeighborRemove();
    Game::Wait(0.1f);
    PLAY_SFX(sfx_Chapter_Victory);
    vv = new(vvBuffer) VictoryView(&Game::cubes[0], Game::previousPuzzle->chapterIndex);
    Game::Wait(1);
    vv->Open();
    Game::Wait(3.5f);
    vv->EndUpdates();

    vv->GetCube()->HideSprites();

    nv->SetCube(&Game::cubes[0]);
    Game::Wait(0.5f);

    isLast = Game::previousPuzzle->chapterIndex == Database::NumChapters()-1;
    if (isLast && Game::saveData.AllChaptersSolved()) {
        nv->SetMessage("You solved\nall the codes!");
        Game::Wait(2.5f);
        nv->SetMessage("Congratulations!", NarratorView::EmoteYay);
        Game::Wait(2.25f);
        nv->SetMessage("We'll go to\nthe home screen now.");
        Game::Wait(2.75f);
        nv->SetMessage("You can replay\nany chapter.");
        Game::Wait(2.75f);
        nv->SetMessage("Or I can create\nrandom puzzles for you!", NarratorView::EmoteMix01);
        int i = 0;
        Game::Wait(0);
        for(float t=0; t<3; t+=Game::dt) {
            i = 1-i;
            if(i==0)
            {
                nv->SetEmote(NarratorView::EmoteMix01);
            }
            else
            {
                nv->SetEmote(NarratorView::EmoteMix02);
            }
            Game::Wait(0);
        }
        nv->SetMessage("Thanks for playing!", NarratorView::EmoteYay);
        Game::Wait(3);
    } else {
        nv->SetMessage("Are you ready for\nthe next chapter?", NarratorView::EmoteMix01);
        int i = 0;
        Game::Wait(0);
        for(float t=0; t<3; t+=Game::dt) {
            i = 1 - i;
            if(i==0)
            {
                nv->SetEmote(NarratorView::EmoteMix01);
            }
            else
            {
                nv->SetEmote(NarratorView::EmoteMix02);
            }
            Game::Wait(0);
        }
        nv->SetMessage("Let's go!", NarratorView::EmoteYay);
        Game::Wait(2.5f);
    }
    nv->SetMessage("");

    AudioPlayer::PlayShutterClose();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        nv->SetTransitionAmount(1-t/kTransitionTime);
        Game::Wait(0);
    }
    nv->SetTransitionAmount(0);

    Game::Wait(0.5f);
    Game::ClearCubeEventHandlers();

#endif //!DISABLE_CHAPTERS
    return Game::GameState_IsOver;
}


}
}
