#include "MenuController.h"
#include "AudioPlayer.h"
#include "TiltFlowItem.h"
#include "TiltFlowMenu.h"
#include "TiltFlowView.h"
#include "ConfirmationMenu.h"
#include "TiltFlowDetailView.h"
#include "BlankView.h"

namespace TotalsGame {
namespace MenuController
{

static const int MAX_CHAPTERS=7;

class TextFieldView : View
{
public:
    void Paint(TotalsCube *c);
};





/* todo lost and found
        void OnCubeLost(Cube cube) {
          if (!cube.IsUnused()) {
            cube.MoveViewTo(mGame.CubeSet.FindAnyIdle());
          }
        } */


void MenuController::TextFieldView::Paint(TotalsCube *c)
{
    /* TODO render text
        c.FillScreen(new Color(200, 200, 200));
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

bool MenuController::TransitionView::GetIsLastFrame() {
    return mOffset == 7+6;
}


void MenuController::TransitionView::Paint()
{
    TotalsCube *c = GetCube();
    if (mOffset < 7) {
        // moving to the left
        c->ClipImage(&VaultDoor, Vec2(7-mOffset, 7));
        c->ClipImage(&VaultDoor, Vec2(7-16-mOffset, 7));
        c->ClipImage(&VaultDoor, Vec2(7-mOffset, 7-16));
        c->ClipImage(&VaultDoor, Vec2(7-16-mOffset,7-16));
    } else {
        // opening
        int off = mOffset - 7;
        int top = 7 - off;
        int bottom = 7 + MIN(5, off);

        if (!mBackwards && bottom-top > 0)
        {
            c->FillArea(&Dark_Purple, Vec2(0, top), Vec2(16, bottom-top));
        }

        c->ClipImage(&VaultDoor, Vec2(0, top-16));
        c->ClipImage(&VaultDoor, Vec2(0, bottom));
    }
}

Game::GameState Run()
{
    TransitionView *tv;
    TiltFlowDetailView *labelView;
    TiltFlowMenu *menu;


    static const float kDuration = 0.333f;
    int numInitialItems=0;
    const char *result = NULL;

    PLAY_MUSIC(sfx_PeanosVaultMenu);


    // transition in
    static char tvBuffer[sizeof(TransitionView)];
    tv = new(tvBuffer) TransitionView(&Game::cubes[0]);

    static char labelBuffer[sizeof(TiltFlowDetailView)];
    labelView = new(labelBuffer) TiltFlowDetailView(&Game::cubes[1]);

    for(int i=2; i<NUM_CUBES; ++i)
    {
        static char blankViewBuffer[NUM_CUBES][sizeof(BlankView)];
        new(blankViewBuffer[i]) BlankView(&Game::cubes[0], NULL);
    }

WelcomeBack:
    labelView->message = "Main Menu";

    AudioPlayer::PlayShutterOpen();
    labelView->TransitionSync(kDuration, true);
    Game::Wait(0.25f);

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv->SetTransitionAmount(t/kDuration);
        Game::Wait(0);
    }
    tv->SetTransitionAmount(1);

    //-----------------------------------------------------------------------
    // WELCOME BACK
    //-----------------------------------------------------------------------

#define ADD_ITEM(graphic, name, desc) do{\
    new (&initialItems[numInitialItems]) TiltFlowItem(&graphic);\
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


    static TiltFlowItem initialItems[5];
    numInitialItems = 0;

    if (Game::currentPuzzle != NULL || !Game::saveData.AllChaptersSolved())
    {
        ADD_ITEM(Icon_Continue, Continue, "Continue from your auto-save data.");
    }

    ADD_ITEM(Icon_Random, RandomPuzzle, "Create a random puzzle!");
    ADD_ITEM(Icon_Howtoplay, Tutorial, "Let Peano teach you how to play!");
#if !DISABLE_CHAPTERS
    ADD_ITEM(Icon_Level_Select, Level, "Replay any level.");
#endif //!DISABLE_CHAPTERS
    ADD_ITEM(Icon_Setup, Setup, "Change your game settings.");
#undef ADD_ITEM

    {
        static TiltFlowItem *items[5]={&initialItems[0], &initialItems[1], &initialItems[2], &initialItems[3], &initialItems[4]};
        static char menuBuffer[sizeof(TiltFlowMenu)];
        menu = new (menuBuffer) TiltFlowMenu(items, numInitialItems, labelView);
        while(!menu->IsDone())
        {
            menu->Tick();
            Game::Wait(0);
        }

        // close labelView
        AudioPlayer::PlayShutterClose();
        labelView->TransitionSync(kDuration, false);
        Game::Wait(0);
        // transition out
        AudioPlayer::PlayShutterClose();
        /* putting the tilt menu on cube 0 secretly deletes the transition view. remake.
                 */
        tv = new(tvBuffer) TransitionView(&Game::cubes[0]);
        //tv->SetCube(menu->GetView()->GetCube());
        for(float t=0; t<kDuration; t+=Game::dt) {
            tv->SetTransitionAmount(1.0f-t/kDuration);
            Game::Wait(0);
        }
        tv->SetTransitionAmount(0);
        Game::Wait(0);

        switch(menu->GetResultItem()->id)
        {
        case Continue:
        {
            if (Game::currentPuzzle == NULL) {
                Game::currentPuzzle = Game::saveData.FindNextPuzzle();
            }

            return Game::GameState_Interstitial;
            break;
        }
        case RandomPuzzle:
        {
            Game::currentPuzzle = NULL;
            return Game::GameState_Interstitial;
            break;
        }
        case Tutorial:
        {
            return Game::GameState_Tutorial;
            break;
        }
        case Level:
        {
            goto ChapterSelect;
            break;  //i like superfluous breaks.
        }
        case Setup:
        {
            goto Setup;
            break;
        }
        }
    }

    //-----------------------------------------------------------------------
    // GAME SETUP OPTIONS
    //-----------------------------------------------------------------------

Setup:

    tv = new(tvBuffer) TransitionView(&Game::cubes[0]);
    labelView = new(labelBuffer) TiltFlowDetailView(&Game::cubes[1]);

    Game::Wait(0.25f);
    labelView->message = "Game Setup";
    AudioPlayer::PlayShutterOpen();
    labelView->TransitionSync(kDuration, true);
    Game::Wait(0.25f);

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv->SetTransitionAmount(t/kDuration);
        Game::Wait(0);
    }
    tv->SetTransitionAmount(1);
    //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

    enum
    {
        Toggle_Difficulty,
        Toggle_Music,
        Toggle_Sfx,
        Clear_Data,
        Back
    };

    {
        static const AssetImage *difficultyIcons[] = {&Icon_Easy, &Icon_Medium, &Icon_Hard};
        static const AssetImage *musicIcons[] = {&Icon_Music_On, &Icon_Music_Off};
        static const AssetImage *sfxIcons[] = {&Icon_Sfx_On, &Icon_Sfx_Off};
        static TiltFlowItem difficultyItems(difficultyIcons, 3);
        static TiltFlowItem musicItems(musicIcons, 2);
        static TiltFlowItem sfxItems(sfxIcons, 2);
        static TiltFlowItem clearDataItem(&Icon_Clear_Data);
        static TiltFlowItem backItem(&Icon_Back);
        static TiltFlowItem *items[] =
        {
            &difficultyItems, &musicItems, &sfxItems, &clearDataItem , &backItem
        };

#define SET_PARAMS(a,b,c,d) a.id=b; a.SetOpt(c); a.description=d;
        SET_PARAMS(difficultyItems, Toggle_Difficulty, (int)Game::difficulty, "Toggle multiplication and division.");
        SET_PARAMS(musicItems, Toggle_Music, AudioPlayer::MusicMuted(), "Toggle background music.");
        SET_PARAMS(sfxItems, Toggle_Sfx, AudioPlayer::SfxMuted(), "Toggle sound effects.");
        SET_PARAMS(clearDataItem, Clear_Data, 0, "Clear your auto-save data.");
        SET_PARAMS(backItem, Back, 0, "Return to the main menu.");
#undef SET_PARAMS

        static char menuBuffer[sizeof(TiltFlowMenu)];
        menu = new (menuBuffer) TiltFlowMenu(items, 5, labelView);

        while(!menu->IsDone())
        {
            menu->Tick();
            Game::Wait(0);
            if (menu->GetToggledItem() != NULL) {
                switch(menu->GetToggledItemIndex())
                {
                case 0:
                    switch(menu->GetToggledItem()->GetOpt())
                    {
                    case 0: Game::difficulty = DifficultyEasy; break;
                    case 1: Game::difficulty = DifficultyMedium; break;
                    case 2: Game::difficulty = DifficultyHard; break;
                    }
                    break;
                case 1:
                    switch(menu->GetToggledItem()->GetOpt())
                    {
                    case 0: AudioPlayer::MuteMusic(false); break;
                    case 1: AudioPlayer::MuteMusic(true); break;
                    }
                    break;
                case 2:
                    switch(menu->GetToggledItem()->GetOpt())
                    {
                    case 0: AudioPlayer::MuteSfx(false); break;
                    case 1: AudioPlayer::MuteSfx(true); break;
                    }
                }

                menu->ClearToggledItem();
            }
        }
    }

    // close labelView
    AudioPlayer::PlayShutterClose();
    labelView->TransitionSync(kDuration, false);
    Game::Wait(0);
    // transition out
    tv = new(tvBuffer) TransitionView(&Game::cubes[0]);
    AudioPlayer::PlayShutterClose();
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv->SetTransitionAmount(1.0f-t/kDuration);
        Game::Wait(0);
    }
    tv->SetTransitionAmount(0);
    Game::Wait(0);

    if (menu->GetResultItem()->id == Back) {
        goto WelcomeBack;
    }


    //-----------------------------------------------------------------------
    // CONFIRM CLEAR DATA
    //-----------------------------------------------------------------------
    {
        if (ConfirmationMenu::Run("Clear Data?")) {
            Game::saveData.Reset();
        }
        Game::Wait(0);
        tv->SetCube(&Game::cubes[0]);
        labelView->SetCube(&Game::cubes[1]);
        goto Setup;
    }



    //-----------------------------------------------------------------------
    // CHAPTER SELECT
    //-----------------------------------------------------------------------

ChapterSelect:
#if !DISABLE_CHAPTERS
    Game::Wait(0.25f);
    labelView->message = "Select a Level";
    AudioPlayer::PlayShutterOpen();
    labelView->TransitionSync(kDuration, true);
    Game::Wait(0.25f);

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv->SetTransitionAmount(t/kDuration);
        Game::Wait(0);
    }
    tv->SetTransitionAmount(1);
    //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

    {
        static char chapterItemBuffer[MAX_CHAPTERS][sizeof(TiltFlowItem)];
        static TiltFlowItem *chapterItems[MAX_CHAPTERS+1] = {0};
        static int numChapterItems;
        numChapterItems = 0;

        for(int i=0; i<Database::NumChapters(); ++i) {
            // only show chapters which can be played with the current cubeset
            if (Database::CanBePlayedWithCurrentCubeSet(i)) {
                if (Game::saveData.IsChapterUnlockedWithCurrentCubeSet(i)) {
                    TiltFlowItem *item = new(chapterItemBuffer[numChapterItems]) TiltFlowItem(&Database::ImageForChapter(i));
                    item->id = i;
                    item->description="Replay this level from the beginning." ;
                    chapterItems[numChapterItems] = item;
                    numChapterItems++;
                } else {
                    TiltFlowItem *item = new(chapterItemBuffer[numChapterItems]) TiltFlowItem(&Icon_Locked);
                    item->id = TiltFlowItem::Passive;
                    item->description="Unlock this level by solving the previous levels." ;
                    chapterItems[numChapterItems] = item;
                    numChapterItems++;
                }
            }
        }

        TiltFlowItem *item = new(chapterItemBuffer[numChapterItems]) TiltFlowItem(&Icon_Back);
        item->id = 1977;  //anything to differentiate NULL from 0.  1977 is NULL
        item->description="Return to the main menu.";
        chapterItems[numChapterItems] = item;
        numChapterItems++;

        static char menuBuffer[sizeof(TiltFlowMenu)];
        menu = new(menuBuffer) TiltFlowMenu (chapterItems, numChapterItems, labelView);
    }

    while(!menu->IsDone()) {
        menu->Tick();
        Game::Wait(0);
    }

    // close labelView
    AudioPlayer::PlayShutterClose();
    labelView->TransitionSync(kDuration, false);
    Game::Wait(0);

    // transition out
    AudioPlayer::PlayShutterClose();
    tv = new(tvBuffer) TransitionView(&Game::cubes[0]);
    for(float t=0; t<kDuration; t+=Game::dt) {
        tv->SetTransitionAmount(1.0f-t/kDuration);
        Game::Wait(0);
    }
    tv->SetTransitionAmount(0);
    Game::Wait(0);

    if (menu->GetResultItem()->id == 1977) {
        goto WelcomeBack;
    }
    else
    {
        int chapter = menu->GetResultItem()->id;
        if(chapter < Database::NumChapters())
        {
            int first = Database::FirstPuzzleForCurrentCubeSetInChapter(chapter);
            Game::currentPuzzle = Database::GetPuzzleInChapter(chapter, first);
        }
        else
        {
            goto WelcomeBack;
        }
    }


#endif //!DISABLE_CHAPTERS

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return Game::GameState_Interstitial;
}


}
}

