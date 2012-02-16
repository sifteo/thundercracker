#include "MenuController.h"
#include "AudioPlayer.h"
#include "TiltFlowItem.h"
#include "TiltFlowMenu.h"

namespace TotalsGame {


DEFINE_POOL(MenuController::TransitionView);


      void MenuController::TextFieldView::Paint(TotalsCube *c)
      {
/* TODO        c.FillScreen(new Color(200, 200, 200));
        Library.Verdana.PaintCentered(c, "(reinforce choice)", 64, 64);
        */
      }



        MenuController::TransitionView::TransitionView(TotalsCube *c): View(c)
        {
            mOffset = 0;
            mBackwards = false;
        }

      void MenuController::TransitionView::SetTransitionAmount(float u)
      {
        // 0 <= offset <= 136+2*kPad (56, kPad, 48)
        SetTransition((7+6+kPad) * u);
      }

      void MenuController::TransitionView::SetTransition(int offset)
      {
        offset = CollapsesPauses(offset);
        offset = MAX(offset, 0);
        offset = MIN(offset, 7+6);
        if (mOffset != offset)
        {
          mBackwards = offset < mOffset;
          mOffset = offset;
          //TODO Paint();
        }
      }

      int MenuController::TransitionView::CollapsesPauses(int off)
      {
        if (off > 7) {
          if (off < 7 + kPad) {
            return 7;
          }
          off -= kPad;
        }
        return off;
      }

      void MenuController::TransitionView::Paint (TotalsCube *c)
      {
        if (mOffset < 7) {
          // moving to the left
            c->Image(&VaultDoor, Vec2(7-mOffset, 7), Vec2(0,0), Vec2(16-7+mOffset, 16-7));
            c->Image(&VaultDoor, Vec2(0, 7), Vec2(16-mOffset, 0), Vec2(7-mOffset, 16-7));
            c->Image(&VaultDoor, Vec2(7-mOffset, 0), Vec2(0,16-7), Vec2(9+mOffset, 7));
            c->Image(&VaultDoor, Vec2(0,0), Vec2(9+mOffset,9), Vec2(7-mOffset,7));
        } else {
          // opening
          int off = mOffset - 7;
          int top = 7 - off;
          int bottom = 7 + MIN(5, off);
          c->Image(&VaultDoor, Vec2(0, 0), Vec2(0, 16-top), Vec2(16, top));
          if (!mBackwards && bottom-top > 0)
          {
          //TODO  c.FillRect(new Color(75, 0, 85), 0, top, 16, bottom-top);
          }
          c->Image(&VaultDoor, Vec2(0, bottom), Vec2(0,0), Vec2(16,16-bottom));
        }
      }



    Game *MenuController::GetGame()
    {
        return mGame;
    }

    MenuController::MenuController(Game *game)
    {
      mGame = game;
    }

    void MenuController::OnSetup()
    {
        CORO_RESET;
      AudioPlayer::PlayMusic(sfx_PeanosVaultMenu);
      // todo mGame.CubeSet.LostCubeEvent += OnCubeLost;
    }
/* todo
    void OnCubeLost(Cube cube) {
      if (!cube.IsUnused()) {
        cube.MoveViewTo(mGame.CubeSet.FindAnyIdle());
      }
    } */

    float MenuController::Coroutine(float dt)
    {
        static const float kDuration = 0.333f;

        CORO_BEGIN



      // transition in
        tv = new TransitionView(Game::GetCube(0));
        labelView = new TiltFlowDetailView(Game::GetCube(1));
      for(int i=2; i<Game::NUMBER_OF_CUBES; ++i)
      {
          new BlankView(Game::GetCube(i), NULL);
      }

    WelcomeBack:
      labelView->message = "Main Menu";


      AudioPlayer::PlayShutterOpen();
      for(rememberedT=0; rememberedT<kDuration; rememberedT=mGame->dt)
      {
        labelView->SetTransitionAmount(rememberedT/kDuration);
        CORO_YIELD(0);
      }
      labelView->SetTransitionAmount(1);
      CORO_YIELD(0.25f);

      AudioPlayer::PlayShutterOpen();
      for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
        tv->SetTransitionAmount(rememberedT/kDuration);
        CORO_YIELD(0);
      }
      tv->SetTransitionAmount(1);

      //-----------------------------------------------------------------------
      // WELCOME BACK
      //-----------------------------------------------------------------------

#define ADD_ITEM(graphic, name, desc)\
    new (&initialItems[numInitialItems]) TiltFlowItem(&graphic);\
    initialItems[numInitialItems].userData = name;\
    initialItems[numInitialItems].description = desc;\
    numInitialItems++;

      static TiltFlowItem initialItems[5];
      int numInitialItems = 0;
      if (mGame->currentPuzzle != NULL /* TODO || !mGame->saveData.AllChaptersSolved() */)
      {
          ADD_ITEM(Icon_Continue, 0,0) //"continue", "Continue from your auto-save data.")
      }

      ADD_ITEM(Icon_Random, "random", "Create a random puzzle!")
      ADD_ITEM(Icon_Howtoplay, "tutorial", "Let Peano teach you how to play!")
      ADD_ITEM(Icon_Level_Select, "level", "Replay any level.")
      ADD_ITEM(Icon_Setup, "setup", "Change your game settings.")

#undef ADD_ITEM

      {
          static char menuBuffer[sizeof(TiltFlowMenu)];
          menu = new (menuBuffer) TiltFlowMenu(gGame, initialItems, numInitialItems);
          while(!tiltFlowMenu->IsDone())
          {
          menu->Tick(mGame->dt);
          CORO_YIELD(0);
        }
#if 0
        // close labelView
        Jukebox.PlayShutterClose();
        for(var t=0f; t<kDuration; t+=mGame.dt) {
          labelView.SetTransitionAmount(1-t/kDuration);
          yield return 0;
        }
        labelView.SetTransitionAmount(0);
        yield return 0;

        // transition out
        Jukebox.PlayShutterClose();
        tv.Cube = menu.view.Cube;
        for(var t=0f; t<kDuration; t+=mGame.dt) {
          tv.SetTransitionAmount(1f-t/kDuration);
          yield return 0;
        }
        tv.SetTransitionAmount(0f);
        yield return 0;

        var result = (string) menu.ResultItem.userData;
        if (result == "continue") {
          if (mGame.currentPuzzle == null) {
            mGame.currentPuzzle = mGame.saveData.FindNextPuzzle();
          }
          mGame.sceneMgr.QueueTransition("Play");
          yield break;
       } else if (menu.ResultItem.userData.ToString() == "random") {
          mGame.currentPuzzle = null;
          mGame.sceneMgr.QueueTransition("Play");
          yield break;
        } else if (result == "tutorial") {
          mGame.sceneMgr.QueueTransition("Tutorial");
          yield break;
        } else if (result == "level") {
          goto ChapterSelect;
        } else if (result == "setup") {
          goto Setup;
        }
#endif
      }

#if 0 //TODO
      //-----------------------------------------------------------------------
      // GAME SETUP OPTIONS
      //-----------------------------------------------------------------------

    Setup:
      yield return 0.25f;
      labelView.message = "Game Setup";
      Jukebox.PlayShutterOpen();
      for(var t=0f; t<kDuration; t+=mGame.dt) {
        labelView.SetTransitionAmount(t/kDuration);
        yield return 0;
      }
      labelView.SetTransitionAmount(1);
      yield return 0.25f;

      Jukebox.PlayShutterOpen();
      for(var t=0f; t<kDuration; t+=mGame.dt) {
        tv.SetTransitionAmount(t/kDuration);
        yield return 0;
      }
      tv.SetTransitionAmount(1);
      //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

      using (var menu = new TiltFlowMenu(
        mGame,
        new TiltFlowItem("icon_easy", "icon_medium", "icon_hard") {
          userData = "toggle_difficulty",
          Opt = (int)mGame.difficulty,
          description = "Toggle multiplication and division."
        },
        new TiltFlowItem("icon_music_on", "icon_music_off") {
          userData = "toggle_music",
          Opt = Jukebox.MuteMusic ? 1 : 0,
          description = "Toggle background music."
        },
        new TiltFlowItem("icon_sfx_on", "icon_sfx_off") {
          userData = "toggle_sfx",
          Opt = Jukebox.MuteSFX ? 1 : 0,
          description = "Toggle sound effects."
        },
        new TiltFlowItem("icon_clear_data") { userData = "clear_data", description = "Clear your auto-save data." },
        new TiltFlowItem("icon_back") { userData = "back", description = "Return to the main menu." }
      )) {
        while(!menu.Done) {
          menu.Tick(mGame.dt);
          yield return 0;
          if (menu.ToggledItem != null) {
            switch(menu.ToggledItem.Image) {
            case "icon_easy": mGame.difficulty = Difficulty.Easy; break;
            case "icon_medium": mGame.difficulty = Difficulty.Medium; break;
            case "icon_hard": mGame.difficulty = Difficulty.Hard; break;
            case "icon_music_on": Jukebox.MuteMusic = false; break;
            case "icon_music_off": Jukebox.MuteMusic = true; break;
            case "icon_sfx_on": Jukebox.MuteSFX = false; break;
            case "icon_sfx_off": Jukebox.MuteSFX = true; break;
            }
            menu.ClearToggledItem();
          }
        }

        // close labelView
        Jukebox.PlayShutterClose();
        for(var t=0f; t<kDuration; t+=mGame.dt) {
          labelView.SetTransitionAmount(1-t/kDuration);
          yield return 0;
        }
        labelView.SetTransitionAmount(0);
        yield return 0;
        // transition out
        Jukebox.PlayShutterClose();
        tv.Cube = menu.view.Cube;
        for(var t=0f; t<kDuration; t+=mGame.dt) {
          tv.SetTransitionAmount(1f-t/kDuration);
          yield return 0;
        }
        tv.SetTransitionAmount(0f);
        yield return 0;

        if (menu.ResultItem.userData.ToString() == "back") {
          goto WelcomeBack;
        }
      }

      //-----------------------------------------------------------------------
      // CONFIRM CLEAR DATA
      //-----------------------------------------------------------------------

      using (var confirm = new ConfirmationMenu("Clear Data?", mGame)) {
        while(!confirm.IsDone) {
          confirm.Tick(mGame.dt);
          yield return 0;
        }
        if (confirm.Result) {
          mGame.saveData.Reset();
        }
        yield return 0;
        tv.Cube = mGame.CubeSet[0];
        labelView.Cube = mGame.CubeSet[1];
        goto Setup;
      }

      //-----------------------------------------------------------------------
      // CHAPTER SELECT
      //-----------------------------------------------------------------------

    ChapterSelect:
      yield return 0.25f;
      labelView.message = "Select a Level";
      Jukebox.PlayShutterOpen();
      for(var t=0f; t<kDuration; t+=mGame.dt) {
        labelView.SetTransitionAmount(t/kDuration);
        yield return 0;
      }
      labelView.SetTransitionAmount(1);
      yield return 0.25f;

      Jukebox.PlayShutterOpen();
      for(var t=0f; t<kDuration; t+=mGame.dt) {
        tv.SetTransitionAmount(t/kDuration);
        yield return 0;
      }
      tv.SetTransitionAmount(1);
      //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

      var csItems = new List<TiltFlowItem>();
      for(int i=0; i<mGame.database.Chapters.Count; ++i) {
        var chapter = mGame.database.Chapters[i];
        // only show chapters which can be played with the current cubeset
        if (chapter.CanBePlayedWithCurrentCubeSet()) {
          if (mGame.saveData.IsChapterUnlockedWithCurrentCubeSet(i)) {
            csItems.Add(new TiltFlowItem("icon_" + chapter.id) { userData = i, description="Replay this level from the beginning." });
          } else {
            // TODO: special locked versions of each icon?
            csItems.Add(new TiltFlowItem("icon_locked") { userData = TiltFlowItem.Passive, description="Unlock this level by solving the previous levels." });
          }
        }
      }
      csItems.Add(new TiltFlowItem("icon_back") { description = "Return to the main menu." });
      using(var menu = new TiltFlowMenu(mGame, csItems.ToArray())) {
        while(!menu.Done) {
          menu.Tick(mGame.dt);
          yield return 0;
        }

        // close labelView
        Jukebox.PlayShutterClose();
        for(var t=0f; t<kDuration; t+=mGame.dt) {
          labelView.SetTransitionAmount(1-t/kDuration);
          yield return 0;
        }
        labelView.SetTransitionAmount(0);
        yield return 0;

        // transition out
        Jukebox.PlayShutterClose();
        tv.Cube = menu.view.Cube;
        for(var t=0f; t<kDuration; t+=mGame.dt) {
          tv.SetTransitionAmount(1f-t/kDuration);
          yield return 0;
        }
        tv.SetTransitionAmount(0f);
        yield return 0;

        if (menu.ResultItem.userData == null) {
          goto WelcomeBack;
        }else {
          try {
            var chapter = (int) menu.ResultItem.userData;
            mGame.currentPuzzle = mGame.database.Chapters[chapter].FirstPuzzleForCurrentCubeSet();
          } catch {
            goto WelcomeBack;
          }
        }
      }
#endif
      mGame->sceneMgr.QueueTransition("Play");

      CORO_END

      return -1;
    }

    float MenuController::OnTick(float dt)
    {
        float r = Coroutine(dt);
        Game::UpdateCubeViews(dt);
        return r;
    }

    void MenuController::OnPaint(bool canvasDirty)
    {
      if (canvasDirty)
      {
          Game::PaintCubeViews();
      }
    }

    void MenuController::OnDispose()
    {
        Game::ClearCubeEventHandlers();
        Game::ClearCubeViews();
    }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

}

