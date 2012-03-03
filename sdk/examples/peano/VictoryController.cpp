#include "VictoryController.h"
#include "NarratorView.h"
#include "VictoryView.h"
#include "BlankView.h"

namespace TotalsGame
{
namespace VictoryController
{
/* TODO
 mGame.CubeSet.NewCubeEvent += cb => new BlankView(cb);
 mGame.CubeSet.LostCubeEvent += OnLostCube;
 */

/* TODO
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

Game::GameState Coroutine() {

    NarratorView *nv;
    VictoryView *vv;

    bool isLast;

    static char nvBuffer[sizeof(NarratorView)];
    static char blankBuffers[NUM_CUBES][sizeof(BlankView)];
    static char vvBuffer[sizeof(VictoryView)];

    nv = new(nvBuffer) NarratorView(&Game::cubes[0]);

    for(int i=1; i<NUM_CUBES; ++i) {
        new(blankBuffers) BlankView(&Game::cubes[0], NULL);
    }
    static const float kTransitionTime = 0.2f;
    CORO_YIELD(0.5f);
    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        nv->SetTransitionAmount(t/kTransitionTime);
        nv->Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    nv->SetTransitionAmount(-1);
    CORO_YIELD(0.25f);
    nv->SetMessage("Codes accepted!\nGet ready!", NarratorView::EmoteYay);
    CORO_YIELD(3);
    nv->SetMessage("");

    AudioPlayer::PlayNeighborRemove();
    CORO_YIELD(0.1f);
    //TODO     PLAY_SFX(sfx_Chapter_Victory);
    vv = new(vvBuffer) VictoryView(&Game::cubes[0], Game::previousPuzzle->chapterIndex);
    CORO_YIELD(1);
    vv->Open();
    //Jukebox.Sfx("win");
    CORO_YIELD(3.5f);
    vv->EndUpdates();
    for(int i = 0; i < _SYS_VRAM_SPRITES; i++)
    {
        vv->GetCube()->backgroundLayer.hideSprite(i);
    }

    nv->SetCube(&Game::cubes[0]);
    CORO_YIELD(0.5f);

    isLast = Game::previousPuzzle->chapterIndex == Database::NumChapters()-1;
    if (isLast && Game::saveData.AllChaptersSolved()) {
        nv->SetMessage("You solved\nall the codes!");
        CORO_YIELD(2.5f);
        nv->SetMessage("Congratulations!", NarratorView::EmoteYay);
        CORO_YIELD(2.25f);
        nv->SetMessage("We'll go to\nthe home screen now.");
        CORO_YIELD(2.75f);
        nv->SetMessage("You can replay\nany chapter.");
        CORO_YIELD(2.75f);
        nv->SetMessage("Or I can create\nrandom puzzles for you!", NarratorView::EmoteMix01);
        int i = 0;
        CORO_YIELD(0);
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
            CORO_YIELD(0);
        }
        nv->SetMessage("Thanks for playing!", NarratorView::EmoteYay);
        CORO_YIELD(3);
    } else {
        nv->SetMessage("Are you ready for\nthe next chapter?", NarratorView::EmoteMix01);
        int i = 0;
        CORO_YIELD(0);
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
            CORO_YIELD(0);
        }
        nv->SetMessage("Let's go!", NarratorView::EmoteYay);
        CORO_YIELD(2.5f);
    }
    nv->SetMessage("");

    AudioPlayer::PlayShutterClose();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        nv->SetTransitionAmount(1-t/kTransitionTime);
        CORO_YIELD(0);
    }
    nv->SetTransitionAmount(0);

    CORO_YIELD(0.5f);


    /* TODO
     mGame.CubeSet.ClearEvents();
     mGame.CubeSet.ClearUserData();
     */

    return Game::GameState_IsOver;
}


}
}
