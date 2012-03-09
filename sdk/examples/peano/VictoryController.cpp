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

    bool isLast;

    NarratorView nv;
    Game::cubes[0].SetView(&nv);

    BlankView blankViews[NUM_CUBES];
    for(int i=1; i<NUM_CUBES; ++i) {
        Game::cubes[i].SetView(blankViews+i);
    }
    static const float kTransitionTime = 0.2f;
    Game::Wait(0.5f);
    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        nv.SetTransitionAmount(t/kTransitionTime);
        nv.Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    nv.SetTransitionAmount(-1);
    Game::Wait(0.25f);
    nv.SetMessage("Codes accepted!\nGet ready!", NarratorView::EmoteYay);
    Game::Wait(3);
    nv.SetMessage("");
    nv.Paint();

    AudioPlayer::PlayNeighborRemove();
    Game::Wait(0.1f);
    PLAY_SFX(sfx_Chapter_Victory);
    VictoryView vv(Game::previousPuzzle->chapterIndex);
    Game::cubes[0].SetView(&vv);
    Game::Wait(1);
    vv.Open();
    float time = 3.5f + System::clock();
    while(System::clock() < time)
    {
        vv.Update();
        vv.Paint();
        System::paint();
        Game::UpdateDt();
    }
    vv.EndUpdates();

    vv.GetCube()->HideSprites();

    Game::cubes[0].SetView(&nv);
    Game::Wait(0.5f);

    isLast = Game::previousPuzzle->chapterIndex == Database::NumChapters()-1;
    if (isLast && Game::saveData.AllChaptersSolved()) {
        nv.SetMessage("You solved\nall the codes!");
        Game::Wait(2.5f);
        nv.SetMessage("Congratulations!", NarratorView::EmoteYay);
        Game::Wait(2.25f);
        nv.SetMessage("We'll go to\nthe home screen now.");
        Game::Wait(2.75f);
        nv.SetMessage("You can replay\nany chapter.");
        Game::Wait(2.75f);
        nv.SetMessage("Or I can create\nrandom puzzles for you!", NarratorView::EmoteMix01);
        {
            Game::Wait(0);
            System::paintSync();
            nv.GetCube()->backgroundLayer.set();
            nv.GetCube()->backgroundLayer.clear();
            nv.GetCube()->foregroundLayer.Clear();
            nv.GetCube()->foregroundLayer.Flush();
            nv.GetCube()->backgroundLayer.setWindow(72,56);

            float t = 3 + System::clock();
            float timeout = 0.0;
            int i=0;
            while(System::clock() < t) {
                timeout -= Game::dt;
                while (timeout < 0) {
                    i = 1 - i;
                    timeout += 0.05;
                    nv.GetCube()->Image(i?&Narrator_Mix02:&Narrator_Mix01, Vec2(0, 0), Vec2(0,3), Vec2(16,7));
                }
                System::paintSync();
                Game::UpdateDt();
            }
        }
        nv.SetMessage("Thanks for playing!", NarratorView::EmoteYay);
        Game::Wait(3);
    } else {
        nv.SetMessage("Are you ready for\nthe next chapter?", NarratorView::EmoteMix01);
        {
            Game::Wait(0);
            System::paintSync();
            nv.GetCube()->backgroundLayer.set();
            nv.GetCube()->backgroundLayer.clear();
            nv.GetCube()->foregroundLayer.Clear();
            nv.GetCube()->foregroundLayer.Flush();
            nv.GetCube()->backgroundLayer.setWindow(72,56);

            float t = 3 + System::clock();
            float timeout = 0.0;
            int i=0;
            while(System::clock() < t) {
                timeout -= Game::dt;
                while (timeout < 0) {
                    i = 1 - i;
                    timeout += 0.05;
                    nv.GetCube()->Image(i?&Narrator_Mix02:&Narrator_Mix01, Vec2(0, 0), Vec2(0,3), Vec2(16,7));
                }
                System::paintSync();
                Game::UpdateDt();
            }
        }
        nv.SetMessage("Let's go!", NarratorView::EmoteYay);
        Game::Wait(2.5f);
    }
    nv.SetMessage("");

    AudioPlayer::PlayShutterClose();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        nv.SetTransitionAmount(1-t/kTransitionTime);
        nv.Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    nv.SetTransitionAmount(0);

    Game::Wait(0.5f);
    Game::ClearCubeEventHandlers();

#endif //!DISABLE_CHAPTERS
    return Game::GameState_IsOver;
}


}
}
