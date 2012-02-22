#include "MenuController.h"
#include "AudioPlayer.h"
#include "TiltFlowItem.h"
#include "TiltFlowMenu.h"
#include "TiltFlowView.h"
#include "ConfirmationMenu.h"



namespace TotalsGame {


//DEFINE_POOL(MenuController::TransitionView);


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
            c->FillScreen(&Dark_Purple);//, Vec2(0, top), Vec2(0,0), Vec2(16, bottom-top));
        }

        c->ClipImage(&VaultDoor, Vec2(0, top-16));
        c->ClipImage(&VaultDoor, Vec2(0, bottom));
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
    int numInitialItems=0;
    const char *result = NULL;

    CORO_BEGIN;



    // transition in
    static char tvBuffer[sizeof(TransitionView)];
    tv = new(tvBuffer) TransitionView(Game::GetCube(0));

    static char labelBuffer[sizeof(TiltFlowDetailView)];
    labelView = new(labelBuffer) TiltFlowDetailView(Game::GetCube(1));

    for(int i=2; i<Game::NUMBER_OF_CUBES; ++i)
    {
        static char blankViewBuffer[Game::NUMBER_OF_CUBES][sizeof(BlankView)];
        new(blankViewBuffer[i]) BlankView(Game::GetCube(i), NULL);
    }

WelcomeBack:
    labelView->message = "Main Menu";

    AudioPlayer::PlayShutterOpen();
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt)
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
    numInitialItems = 0;

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
        static TiltFlowItem *items[5]={&initialItems[0], &initialItems[1], &initialItems[2], &initialItems[3], &initialItems[4]};
        static char menuBuffer[sizeof(TiltFlowMenu)];
        menu = new (menuBuffer) TiltFlowMenu(items, numInitialItems, labelView);
        while(!menu->IsDone())
        {
            menu->Tick(mGame->dt);
            CORO_YIELD(0);
        }

        // close labelView
        AudioPlayer::PlayShutterClose();
        for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
            labelView->SetTransitionAmount(1-rememberedT/kDuration);
            CORO_YIELD(0);
        }
        labelView->SetTransitionAmount(0);
        CORO_YIELD(0);
        // transition out
        AudioPlayer::PlayShutterClose();
        /* putting the tilt menu on cube 0 secretly deletes the transition view. remake.
          */
        tv = new(tvBuffer) TransitionView(Game::GetCube(0));
        //tv->SetCube(menu->GetView()->GetCube());
        for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
            tv->SetTransitionAmount(1.0f-rememberedT/kDuration);
            CORO_YIELD(0);
        }
        tv->SetTransitionAmount(0);
        CORO_YIELD(0);

        result = (const char*)menu->GetResultItem()->userData;
        if (!strcmp(result,"continue")) {
            if (mGame->currentPuzzle == NULL) {
                mGame->currentPuzzle = mGame->saveData.FindNextPuzzle();
            }
            mGame->sceneMgr.QueueTransition("Play");
            CORO_YIELD(0);
        } else if (!strcmp(result,"random")) {
            mGame->currentPuzzle = NULL;
            mGame->sceneMgr.QueueTransition("Play");
            CORO_YIELD(0);
        } else if (!strcmp(result,"tutorial")) {
            mGame->sceneMgr.QueueTransition("Tutorial");
            CORO_YIELD(0);
        } else if (!strcmp(result,"level")) {
            goto ChapterSelect;
        } else if (!strcmp(result,"setup")) {
            goto Setup;
        }
    }

    //-----------------------------------------------------------------------
    // GAME SETUP OPTIONS
    //-----------------------------------------------------------------------

Setup:

    tv = new(tvBuffer) TransitionView(Game::GetCube(0));
    labelView = new(labelBuffer) TiltFlowDetailView(Game::GetCube(1));

    CORO_YIELD(0.25f);
    labelView->message = "Game Setup";
    AudioPlayer::PlayShutterOpen();
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
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
    //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

    {
        static const PinnedAssetImage *difficultyIcons[] = {&Icon_Easy, &Icon_Medium, &Icon_Hard};
        static const PinnedAssetImage *musicIcons[] = {&Icon_Music_On, &Icon_Music_Off};
        static const PinnedAssetImage *sfxIcons[] = {&Icon_Sfx_On, &Icon_Sfx_Off};
        static TiltFlowItem difficultyItems(difficultyIcons, 3);
        static TiltFlowItem musicItems(musicIcons, 2);
        static TiltFlowItem sfxItems(sfxIcons, 2);
        static TiltFlowItem clearDataItem(&Icon_Clear_Data);
        static TiltFlowItem backItem(&Icon_Back);
        static TiltFlowItem *items[] =
        {
            &difficultyItems, &musicItems, &sfxItems, &clearDataItem , &backItem
        };

#define SET_PARAMS(a,b,c,d) a.userData=b; a.SetOpt(c); a.description=d;
        SET_PARAMS(difficultyItems, "toggle_difficulty", (int)mGame->difficulty, "Toggle multiplication and division.");
        SET_PARAMS(musicItems, "toggle_music", AudioPlayer::MusicMuted(), "Toggle background music.");
        SET_PARAMS(sfxItems, "toggle_sfx", AudioPlayer::SfxMuted(), "Toggle sound effects.");
        SET_PARAMS(clearDataItem, "clear_data", 0, "Clear your auto-save data.");
        SET_PARAMS(backItem, "back", 0, "Return to the main menu.");
#undef SET_PARAMS

        static char menuBuffer[sizeof(TiltFlowMenu)];
        menu = new (menuBuffer) TiltFlowMenu(items, 5, labelView);

        while(!menu->IsDone())
        {
            menu->Tick(dt);
            CORO_YIELD(0);
            if (menu->GetToggledItem() != NULL) {
                switch(menu->GetToggledItemIndex())
                {
                case 0:
                    switch(menu->GetToggledItem()->GetOpt())
                    {
                    case 0: mGame->difficulty = DifficultyEasy; break;
                    case 1: mGame->difficulty = DifficultyMedium; break;
                    case 2: mGame->difficulty = DifficultyHard; break;
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
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
        labelView->SetTransitionAmount(1.0f-rememberedT/kDuration);
        CORO_YIELD(0);
    }
    labelView->SetTransitionAmount(0);
    CORO_YIELD(0);
    // transition out
    tv = new(tvBuffer) TransitionView(Game::GetCube(0));
    AudioPlayer::PlayShutterClose();
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
        tv->SetTransitionAmount(1.0f-rememberedT/kDuration);
        CORO_YIELD(0);
    }
    tv->SetTransitionAmount(0);
    CORO_YIELD(0);

    if (!strcmp((const char*)menu->GetResultItem()->userData, "back")) {
        goto WelcomeBack;
    }


    //-----------------------------------------------------------------------
    // CONFIRM CLEAR DATA
    //-----------------------------------------------------------------------
    {
        static char buffer[sizeof(ConfirmationMenu)];
        confirm = new(buffer) ConfirmationMenu("Clear Data?");

        while(!confirm->IsDone()) {
            confirm->Tick(mGame->dt);
            CORO_YIELD(0);
        }
        if (confirm->GetResult()) {
            mGame->saveData.Reset();
        }
        CORO_YIELD(0);
        tv->SetCube(Game::GetCube(0));
        labelView->SetCube(Game::GetCube(1));
        goto Setup;
    }



    //-----------------------------------------------------------------------
    // CHAPTER SELECT
    //-----------------------------------------------------------------------

ChapterSelect:
    CORO_YIELD(0.25f);
    labelView->message = "Select a Level";
    AudioPlayer::PlayShutterOpen();
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
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
    //tv.Cube.Image("tilt_to_select", (128-tts.width)>>1, 128-26);

    {
        static char chapterItemBuffer[MAX_CHAPTERS][sizeof(TiltFlowItem)];
        static TiltFlowItem *chapterItems[MAX_CHAPTERS+1] = {0};
        static int numChapterItems;
        numChapterItems = 0;

        for(int i=0; i<mGame->database.NumChapters(); ++i) {
            PuzzleChapter *chapter = mGame->database.GetChapter(i);
            // only show chapters which can be played with the current cubeset
            if (chapter->CanBePlayedWithCurrentCubeSet()) {
                if (mGame->saveData.IsChapterUnlockedWithCurrentCubeSet(i)) {
                    TiltFlowItem *item = new(chapterItemBuffer[numChapterItems]) TiltFlowItem(chapter->idImage);
                    item->userData = (void*)i;
                    item->description="Replay this level from the beginning." ;
                    chapterItems[numChapterItems] = item;
                    numChapterItems++;
                } else {
                    TiltFlowItem *item = new(chapterItemBuffer[numChapterItems]) TiltFlowItem(&Icon_Locked);
                    item->userData = (void*)TiltFlowItem::Passive;
                    item->description="Unlock this level by solving the previous levels." ;
                    chapterItems[numChapterItems] = item;
                    numChapterItems++;
                }
            }
        }

        TiltFlowItem *item = new(chapterItemBuffer[numChapterItems]) TiltFlowItem(&Icon_Back);
        item->userData = (void*)1977;  //anything to differentiate NULL from 0.  1977 is NULL
        item->description="Return to the main menu.";
        chapterItems[numChapterItems] = item;
        numChapterItems++;

        static char menuBuffer[sizeof(TiltFlowMenu)];
        menu = new(menuBuffer) TiltFlowMenu (chapterItems, numChapterItems, labelView);
    }

    while(!menu->IsDone()) {
        menu->Tick(mGame->dt);
        CORO_YIELD(0);
    }

    // close labelView
    AudioPlayer::PlayShutterClose();
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
        labelView->SetTransitionAmount(1-rememberedT/kDuration);
        CORO_YIELD(0);
    }
    labelView->SetTransitionAmount(0);
    CORO_YIELD(0);

    // transition out
    AudioPlayer::PlayShutterClose();
    tv = new(tvBuffer) TransitionView(Game::GetCube(0));
    for(rememberedT=0; rememberedT<kDuration; rememberedT+=mGame->dt) {
        tv->SetTransitionAmount(1.0f-rememberedT/kDuration);
        CORO_YIELD(0);
    }
    tv->SetTransitionAmount(0);
    CORO_YIELD(0);

    if (menu->GetResultItem()->userData == (void*)1977) {
        goto WelcomeBack;
    }
    else
    {
        int chapter = (int) menu->GetResultItem()->userData;
        if(chapter < mGame->database.NumChapters())
        {
            mGame->currentPuzzle = mGame->database.GetChapter(chapter)->FirstPuzzleForCurrentCubeSet();
        }
        else
        {
            goto WelcomeBack;
        }
    }

    mGame->sceneMgr.QueueTransition("Play");

    CORO_END;

    return -1;
}

void MenuController::OnTick(float dt)
{
    UPDATE_CORO(Coroutine, dt);
    Game::UpdateCubeViews(dt);
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

