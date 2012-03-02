#pragma once

#include "config.h"

#if !DISABLE_CHAPTERS

#include "VictoryView.h"
#include "coroutine.h"
#include "NarratorView.h"

namespace TotalsGame {

  class VictoryController : public IStateController {
    Game *mGame;

    CORO_PARAMS;
    float remembered_t;
      int remembered_i;


    NarratorView *nv;
    VictoryView *vv;

  public:

    VictoryController (Game *game) {
      mGame = game;
    }

    void OnSetup() {
        /* TODO
      mGame.CubeSet.NewCubeEvent += cb => new BlankView(cb);
      mGame.CubeSet.LostCubeEvent += OnLostCube;
      */
    }
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
    float Coroutine() {
        
        bool isLast;
        
        static char nvBuffer[sizeof(NarratorView)];
        static char blankBuffers[Game::NUMBER_OF_CUBES][sizeof(BlankView)];
        static char vvBuffer[sizeof(VictoryView)];

        CORO_BEGIN;

        nv = new(nvBuffer) NarratorView(Game::GetCube(0));

      for(int i=1; i<Game::NUMBER_OF_CUBES; ++i) {
          new(blankBuffers) BlankView(Game::GetCube(i), NULL);
      }
      static const float kTransitionTime = 0.2f;
      CORO_YIELD(0.5f);
      AudioPlayer::PlayShutterOpen();
      for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=Game::dt) {
        nv->SetTransitionAmount(remembered_t/kTransitionTime);
        nv->Paint();
        System::paintSync();
        Game::GetInstance().UpdateDt();
      }
      nv->SetTransitionAmount(-1);
      CORO_YIELD(0.25f);
        nv->SetMessage("Codes accepted!\nGet ready!", NarratorView::EmoteYay);
      CORO_YIELD(3);
      nv->SetMessage("");

      AudioPlayer::PlayNeighborRemove();
        CORO_YIELD(0.1f);
 //TODO     PLAY_SFX(sfx_Chapter_Victory);
      vv = new(vvBuffer) VictoryView(Game::GetCube(0), mGame->previousPuzzle->chapterIndex);
      CORO_YIELD(1);
      vv->Open();
      //Jukebox.Sfx("win");
      CORO_YIELD(3.5f);
      vv->EndUpdates();
      for(int i = 0; i < _SYS_VRAM_SPRITES; i++)
      {
          vv->GetCube()->backgroundLayer.hideSprite(i);
      }

      nv->SetCube(Game::GetCube(0));
      CORO_YIELD(0.5f);

      isLast = mGame->previousPuzzle->chapterIndex == Database::NumChapters()-1;
      if (isLast && mGame->saveData.AllChaptersSolved()) {
        nv->SetMessage("You solved\nall the codes!");
        CORO_YIELD(2.5f);
          nv->SetMessage("Congratulations!", NarratorView::EmoteYay);
        CORO_YIELD(2.25f);
        nv->SetMessage("We'll go to\nthe home screen now.");
        CORO_YIELD(2.75f);
        nv->SetMessage("You can replay\nany chapter.");
        CORO_YIELD(2.75f);
          nv->SetMessage("Or I can create\nrandom puzzles for you!", NarratorView::EmoteMix01);
        remembered_i=0;
        CORO_YIELD(0);
        for(remembered_t=0; remembered_t<3; remembered_t+=mGame->dt) {
          remembered_i = 1-remembered_i;
          if(remembered_i==0)
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
        remembered_i=0;
        CORO_YIELD(0);
        for(remembered_t=0; remembered_t<3; remembered_t+=mGame->dt) {
            remembered_i = 1 - remembered_i;
            if(remembered_i==0)
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
      for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=mGame->dt) {
        nv->SetTransitionAmount(1-remembered_t/kTransitionTime);
       CORO_YIELD(0);
      }
      nv->SetTransitionAmount(0);

      CORO_YIELD(0.5f);

      Advance();

      CORO_END;

      return -1;
    }

    void Advance() {
      mGame->sceneMgr.QueueTransition("Next");
    }

    void OnTick () {
        UPDATE_CORO(Coroutine);
        Game::UpdateCubeViews();
    }

    void OnPaint (bool canvasDirty) {
      if (canvasDirty) { Game::PaintCubeViews(); }
    }

    void OnDispose () {
    /* TODO
      mGame.CubeSet.ClearEvents();
      mGame.CubeSet.ClearUserData();
*/
    }

  };

}

#endif //!DISABLE_CHAPTERS
