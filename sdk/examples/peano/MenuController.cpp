#include "MenuController.h"
#include "AudioPlayer.h"
#include "TiltFlowItem.h"
#include "TiltFlowMenu.h"
#include "TiltFlowView.h"
#include "ConfirmationMenu.h"
#include "TiltFlowDetailView.h"
#include "Skins.h"

namespace TotalsGame {
namespace MenuController
{

static const int MAX_CHAPTERS=7;


/* todo lost and found
        void OnCubeLost(Cube cube) {
          if (!cube.IsUnused()) {
            cube.MoveViewTo(mGame.CubeSet.FindAnyIdle());
          }
        } */


MenuController::TransitionView::TransitionView()
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

        Paint();
        System::paintSync();
    }
    else
    {
        System::yield();
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

bool MenuController::TransitionView::GetIsLastFrame() {
    return mOffset == 7+6;
}


void MenuController::TransitionView::Paint()
{
    const Skins::Skin &skin = Skins::GetSkin();
    TotalsCube *c = GetCube();
    if (mOffset < 7) {
        // moving to the left
        c->ClipImage(&skin.vault_door, Vec2(7-mOffset, 7));
        c->ClipImage(&skin.vault_door, Vec2(7-16-mOffset, 7));
        c->ClipImage(&skin.vault_door, Vec2(7-mOffset, 7-16));
        c->ClipImage(&skin.vault_door, Vec2(7-16-mOffset,7-16));
    } else {
        // opening
        int off = mOffset - 7;
        int top = 7 - off;
        int bottom = 7 + MIN(5, off);

        if (!mBackwards && bottom-top > 0)
        {
            c->FillArea(&Dark_Purple, Vec2(0, top), Vec2(16, bottom-top));
        }

        c->ClipImage(&skin.vault_door, Vec2(0, top-16));
        c->ClipImage(&skin.vault_door, Vec2(0, bottom));
    }
}

enum MenuState
{
    MenuState_WelcomeBack,
    MenuState_Setup,
    MenuState_ChapterSelect
};

MenuState menuState;
Game::GameState nextGameState;

void RunWelcomeBack();
void RunSetup();
void RunChapterSelect();

static const float kDuration = 0.333f;

Game::GameState Run()
{
    PLAY_MUSIC(sfx_PeanosVaultMenu);

    menuState = MenuState_WelcomeBack;

    nextGameState = Game::GameState(500);
    while(nextGameState == Game::GameState(500))
    {
        switch(menuState)
        {
        case MenuState_WelcomeBack:
            RunWelcomeBack();
            break;
        case MenuState_Setup:
            RunSetup();
            break;
        case MenuState_ChapterSelect:
            RunChapterSelect();
            break;
        }
    }

    return nextGameState;
}

int DoMenu(const char *name, TiltFlowItem *items, int nItems)
{
    TransitionView tv;
    Game::cubes[0].SetView(&tv);

    TiltFlowDetailView labelView;
    Game::cubes[1].SetView(&labelView);

    // transition in
    for(int i=2; i<NUM_CUBES; ++i)
    {
        Game::cubes[i].DrawVaultDoorsClosed();
    }

    labelView.message = name;
    labelView.TransitionSync(kDuration, true);
    Game::Wait(0.25f);

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv.SetTransitionAmount(t/kDuration);
        Game::UpdateDt();
    }
    tv.SetTransitionAmount(1);

    TiltFlowView tiltFlowView;
    Game::cubes[0].SetView(&tiltFlowView);
    TiltFlowMenu menu(items, nItems, &tiltFlowView, &labelView);

    while(!menu.IsDone())
    {
        menu.Tick();
        Game::Wait(0);
    }

    Game::ClearCubeEventHandlers();

    // close labelView
    labelView.message = "";
    labelView.HideDescription();
    labelView.TransitionSync(kDuration, false);

    // transition out
    AudioPlayer::PlayShutterClose();
    Game::cubes[0].SetView(&tv);
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv.SetTransitionAmount(1.0f-t/kDuration);
        Game::UpdateDt();
        System::paintSync();
    }
    tv.SetTransitionAmount(0);

    return menu.GetResultItem()->id;
}


void RunWelcomeBack()
{
#define ADD_ITEM(graphic, name, desc) do{\
    initialItems[numInitialItems].SetImage(&graphic);\
    initialItems[numInitialItems].id = name;\
    initialItems[numInitialItems].description = desc;\
    numInitialItems++;}while(0)

    enum
    {
        Continue,
        RandomPuzzle,
        Tutorial,
        Level,
        Setup
    };

    TiltFlowItem initialItems[5];
    int numInitialItems = 0;

    if (Game::currentPuzzle != NULL || !Game::saveData.AllChaptersSolved())
    {
        ADD_ITEM(Icon_Continue, Continue, "Continue from your\nauto-save data.");
    }

    ADD_ITEM(Icon_Random, RandomPuzzle, "Create a random puzzle!");
    ADD_ITEM(Icon_Howtoplay, Tutorial, "Let Peano teachn\nyou how to play!");
    ADD_ITEM(Icon_Level_Select, Level, "Replay any level.");
    ADD_ITEM(Icon_Setup, Setup, "Change your\ngame settings.");
#undef ADD_ITEM

    int resultId = DoMenu("Main Menu", initialItems, numInitialItems);

    switch(resultId)
    {
    case Continue:
    {
        if (Game::currentPuzzle == NULL) {
            Game::currentPuzzle = Game::saveData.FindNextPuzzle();
        }
        else
        {
            //puzzle is still hanging around but the tokens and tokengroups have been cleared
            //recreate it
            int c = Game::currentPuzzle->chapterIndex;
            int p = Game::currentPuzzle->puzzleIndex;
            Game::currentPuzzle = Database::GetPuzzleInChapter(c, p);
        }

        nextGameState = Game::GameState_Interstitial;
        goto end;
    }
    case RandomPuzzle:
    {
        Game::currentPuzzle = NULL;
        nextGameState = Game::GameState_Interstitial;
        goto end;
        break;
    }
    case Tutorial:
    {
        nextGameState = Game::GameState_Tutorial;
        goto end;
        break;
    }
    case Level:
    {
        menuState = MenuState_ChapterSelect;
        goto end;
        break;
    }
    case Setup:
    {
        menuState = MenuState_Setup;
        goto end;
        break;
    }
    }
end:
Game::ClearCubeEventHandlers();
Game::ClearCubeViews();
}
void RunSetup()
{
    TransitionView tv;
    Game::cubes[0].SetView(&tv);

    TiltFlowDetailView labelView;
    Game::cubes[1].SetView(&labelView);

    Game::Wait(0.25f);
    labelView.message = "Game Setup";
    labelView.TransitionSync(kDuration, true);
    Game::Wait(0.25f);

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv.SetTransitionAmount(t/kDuration);
        Game::UpdateDt();
    }
    tv.SetTransitionAmount(1);
    //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

    enum
    {
        Toggle_SkillLevel,
        Toggle_Music,
        Toggle_Sfx,
        Clear_Data,
        Back
    };

    {
        static const Sifteo::AssetImage *skillIcons[] = {&Icon_Novice, &Icon_Expert};
        static const Sifteo::AssetImage *musicIcons[] = {&Icon_Music_On, &Icon_Music_Off};
        static const Sifteo::AssetImage *sfxIcons[] = {&Icon_Sfx_On, &Icon_Sfx_Off};

        TiltFlowItem items[5];

        items[0].SetImages(skillIcons, 2);
        items[1].SetImages(musicIcons, 2);
        items[2].SetImages(sfxIcons, 2);
        items[3].SetImage(&Icon_Clear_Data);
        items[4].SetImage(&Icon_Back);

#define SET_PARAMS(a,b,c,d) a.id=b; a.SetOpt(c); a.description=d;
        SET_PARAMS(items[0], Toggle_SkillLevel, (int)Game::skillLevel, "Toggle Mathematical Difficulty.");
        SET_PARAMS(items[1], Toggle_Music, AudioPlayer::MusicMuted(), "Toggle\nbackground music.");
        SET_PARAMS(items[2], Toggle_Sfx, AudioPlayer::SfxMuted(), "Toggle sound effects.");
        SET_PARAMS(items[3], Clear_Data, 0, "Clear your\nauto-save data.");
        SET_PARAMS(items[4], Back, 0, "Return to the main menu.");
#undef SET_PARAMS

        TiltFlowView tiltFlowView;
        Game::cubes[0].SetView(&tiltFlowView);
        TiltFlowMenu menu(items, 5, &tiltFlowView, &labelView);

        while(!menu.IsDone())
        {
            menu.Tick();
            Game::Wait(0);
            if (menu.GetToggledItem() != NULL) {
                switch(menu.GetToggledItemIndex())
                {
                case 0:
                    switch(menu.GetToggledItem()->GetOpt())
                    {
                    case 0: Game::skillLevel = Game::SkillLevel_Novice; break;
                    case 1: Game::skillLevel = Game::SkillLevel_Expert; break;
                    }
                    break;
                case 1:
                    switch(menu.GetToggledItem()->GetOpt())
                    {
                    case 0: AudioPlayer::MuteMusic(false); break;
                    case 1: AudioPlayer::MuteMusic(true); break;
                    }
                    break;
                case 2:
                    switch(menu.GetToggledItem()->GetOpt())
                    {
                    case 0: AudioPlayer::MuteSfx(false); break;
                    case 1: AudioPlayer::MuteSfx(true); break;
                    }
                }

                Game::SaveOptions();

                menu.ClearToggledItem();
            }
        }

        Game::ClearCubeEventHandlers();

        // close labelView
        labelView.TransitionSync(kDuration, false);
        // transition out
        {
            TransitionView tv;
            Game::cubes[0].SetView(&tv);
            AudioPlayer::PlayShutterClose();
            for(float t=0; t<kDuration; t+=Game::dt) {
                tv.SetTransitionAmount(1.0f-t/kDuration);
                Game::UpdateDt();
            }
        }

        if (menu.GetResultItem()->id == Back)
        {
            menuState = MenuState_WelcomeBack;
            goto end;
        }
    }


    //-----------------------------------------------------------------------
    // CONFIRM CLEAR DATA
    //-----------------------------------------------------------------------
    {
        if (ConfirmationMenu::Run("Clear Data?")) {
            Game::saveData.Reset();
        }
    }
end:
    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();
}

//-----------------------------------------------------------------------
// CHAPTER SELECT
//-----------------------------------------------------------------------

void RunChapterSelect()
{   
    TiltFlowItem chapterItems[MAX_CHAPTERS+1];
    int numChapterItems = 0;

    for(int i=0; i<Database::NumChapters(); ++i) {
        // only show chapters which can be played with the current cubeset
        if (Database::CanBePlayedWithCurrentCubeSet(i)) {
            TiltFlowItem *item = chapterItems+numChapterItems;
            if (Game::saveData.IsChapterUnlockedWithCurrentCubeSet(i)) {
                item->SetImage(&Database::ImageForChapter(i));
                item->id = i;
                item->description="Replay this level\nfrom the beginning." ;
            } else {
                item->SetImage(&Icon_Locked);
                item->id = TiltFlowItem::Passive;
                item->description="Unlock this level\nby solving the previous\nlevels." ;
            }
            numChapterItems++;
        }
    }

    const int BackId = 234; //arbitrary

    TiltFlowItem *item = chapterItems + numChapterItems;
    item->SetImage(&Icon_Back);
    item->id = BackId;
    item->description="Return to the main menu.";
    numChapterItems++;

    int resultId = DoMenu("Select a Level", chapterItems, numChapterItems);

    if (resultId == BackId) {
        menuState = MenuState_WelcomeBack;
        goto end;
    }
    else
    {
        int chapter = resultId;
        if(chapter < Database::NumChapters())
        {
            int first = Database::FirstPuzzleForCurrentCubeSetInChapter(chapter);
            Game::currentPuzzle = Database::GetPuzzleInChapter(chapter, first);
            nextGameState = Game::GameState_Interstitial;
        }
        else
        {
            menuState = MenuState_WelcomeBack;
            goto end;
        }
    }

end:
    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();
}


}
}

