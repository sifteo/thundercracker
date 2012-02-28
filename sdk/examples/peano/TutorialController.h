#pragma once

#include "StateMachine.h"
#include "AudioPlayer.h"
#include "coroutine.h"
#include "Game.h"
#include "NarratorView.h"
#include "Puzzle.h"
#include "assets.gen.h"


namespace TotalsGame {

class TutorialController : public IStateController {

    class ConnectTwoCubesHorizontalEventHandler: public Game::NeighborEventHandler
    {
        TutorialController *owner;
    public:
        ConnectTwoCubesHorizontalEventHandler(TutorialController *_owner)
        {
            owner = _owner;
        }

        void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            if ((s0 == SIDE_RIGHT && s1 == SIDE_LEFT && c1 == owner->secondToken->GetCube()->id())
                    ||(s0 == SIDE_LEFT && s1 == SIDE_RIGHT && c0 == owner->secondToken->GetCube()->id()))
            {
                Game::GetInstance().neighborEventHandler = NULL;
                TokenGroup *grp = TokenGroup::Connect(owner->firstToken->token, kSideToUnit[SIDE_RIGHT], owner->secondToken->token);
                if (grp != NULL)
                {
                    owner->firstToken->WillJoinGroup();
                    owner->secondToken->WillJoinGroup();
                    grp->SetCurrent(grp);
                    owner->firstToken->DidJoinGroup();
                    owner->secondToken->DidJoinGroup();
                }
                //TODO redundant? firstToken.Cube.ClearEvents();
                AudioPlayer::PlaySfx(sfx_Tutorial_Correct);
            }
        }
    };
    ConnectTwoCubesHorizontalEventHandler connectTwoCubesHorizontalEventHandler;


    class ConnectTwoCubesVerticalEventHandler: public Game::NeighborEventHandler
    {
        TutorialController *owner;
    public:
        ConnectTwoCubesVerticalEventHandler(TutorialController *_owner)
        {
            owner = _owner;
        }

        void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            if ((s0 == SIDE_TOP && s1 == SIDE_BOTTOM && c1 == owner->secondToken->GetCube()->id())
                    ||(s0 == SIDE_BOTTOM && s1 == SIDE_TOP && c0 == owner->secondToken->GetCube()->id()))
            {
                Game::GetInstance().neighborEventHandler = NULL;
                TokenGroup *grp = TokenGroup::Connect(owner->firstToken->token, kSideToUnit[SIDE_TOP], owner->secondToken->token);
                if (grp != NULL)
                {
                    owner->firstToken->WillJoinGroup();
                    owner->secondToken->WillJoinGroup();
                    grp->SetCurrent(grp);
                    owner->firstToken->DidJoinGroup();
                    owner->secondToken->DidJoinGroup();
                }
                //TODO redundant? firstToken.Cube.ClearEvents();
                AudioPlayer::PlaySfx(sfx_Tutorial_Correct);
            }
        }
    };
    ConnectTwoCubesVerticalEventHandler connectTwoCubesVerticalEventHandler;

    class MakeSixEventHandler: public Game::NeighborEventHandler
    {
        TutorialController *owner;
    public:
        MakeSixEventHandler(TutorialController *_owner)
        {
            owner = _owner;
        }

        void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            owner->OnNeighborAdd(Game::GetCube(c0), s0, Game::GetCube(c1), s1);
            if (!owner->firstToken->token->current->GetValue() == Fraction(6)) {
                AudioPlayer::PlaySfx(sfx_Tutorial_Oops, false);
                owner->narrator->SetMessage("Oops, let's try again...", NarratorView::EmoteSad);
            }
        }

        void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            owner->OnNeighborRemove(Game::GetCube(c0),s0, Game::GetCube(c1), s1);
            if (owner->firstToken->token->current == owner->firstToken->token) {
                owner->narrator->SetMessage("Can you build the number 6?");
            }
        }
    };
    MakeSixEventHandler makeSixEventHandler;

    class WaitForShakeEventHandler: public TotalsCube::EventHandler
    {
        bool shook;
    public:
        WaitForShakeEventHandler()
        {
            shook = false;
        }

        void OnCubeShake(TotalsCube *cube)
        {
            shook = true;
        }

        bool DidShake()
        {
            return shook;
        }
    };
    WaitForShakeEventHandler waitForShakeEventHandler[2];

    class WaitForTouchEventHanlder: public TotalsCube::EventHandler
    {
        bool touched;
    public:
        WaitForTouchEventHanlder()
        {
            touched = false;
        }

        void OnCubeTouch(TotalsCube *cube, bool _touched)
        {
            touched = touched | _touched;
        }

        bool DidTouch()
        {
            return touched;
        }
    };
    WaitForTouchEventHanlder waitForTouchEventHandler[2];


    CORO_PARAMS;
    float remembered_t;
    Puzzle *puzzle;
    TokenView *firstToken, *secondToken;
    float rememberedTimeout;
    int rememberedCubeId;
    int rememberedFinishCountdown ;

    NarratorView *narrator;
    Game *mGame;
public:
    TutorialController (Game *game):
        connectTwoCubesHorizontalEventHandler(this),
        connectTwoCubesVerticalEventHandler(this),
        makeSixEventHandler(this)
    {
        mGame = game;
    }

    void OnSetup() {
        AudioPlayer::PlayMusic(sfx_PeanosVaultMenu);
        CORO_RESET;
        narrator = NULL;
        puzzle = NULL;
        firstToken = NULL;
        secondToken = NULL;
        /* TODO
        mGame.CubeSet.LostCubeEvent += OnLostCube;
        mGame.CubeSet.NewCubeEvent += OnNewCube;
*/
    }
    /* TODO
    void OnLostCube(Cube c) {
        if (!c.IsUnused()) {
            var blanky = mGame.CubeSet.FindAnyIdle();
            if(blanky == null) { throw new Exception("Fatal Lost Cube"); }
            c.MoveViewTo(blanky);
        }
    }

    void OnNewCube(Cube c) {
        new BlankView(c);
    }
    */
    float Coroutine(float dt) {
        const float kTransitionDuration = 0.2f;
        static char narratorViewBuffer[sizeof(NarratorView)];
        static char blankViewBuffer[Game::NUMBER_OF_CUBES][sizeof(BlankView)];
        static char tokenViewBuffer[2][sizeof(TokenView)];

        const float period = 0.05f;

        CORO_BEGIN;

        // initialize narrator
        narrator = new(narratorViewBuffer) NarratorView(Game::GetCube(0));

        // initial blanks
        for(int i = 1; i < Game::NUMBER_OF_CUBES; i++)
        {
            new(blankViewBuffer[i]) BlankView(Game::GetCube(i), NULL);
        }
        CORO_YIELD(0);

        CORO_YIELD(0.5f);


        AudioPlayer::PlayShutterOpen();
        for(remembered_t=0; remembered_t<kTransitionDuration; remembered_t+=mGame->dt) {
            narrator->SetTransitionAmount(remembered_t/kTransitionDuration);
            CORO_YIELD(0);
        }
        narrator->SetTransitionAmount(-1);

        // wait a bit
        CORO_YIELD(0.5f);
        narrator->SetMessage("Hi there! I'm Peano!", NarratorView::EmoteWave);
        CORO_YIELD(3);
        narrator->SetMessage("We're going to learn\nto solve secret codes!");
        CORO_YIELD(3);
        narrator->SetMessage("These codes let you\ninto the treasure vault!", NarratorView::EmoteYay);
        CORO_YIELD(3);

        // initailize puzzle
        puzzle = new Puzzle(2);
        puzzle->unlimitedHints = true;
        puzzle->GetToken(0)->val = 1;
        puzzle->GetToken(0)->SetOpRight(OpAdd);
        puzzle->GetToken(0)->SetOpBottom(OpSubtract);
        puzzle->GetToken(1)->val = 2;
        puzzle->GetToken(1)->SetOpRight(OpAdd);
        puzzle->GetToken(1)->SetOpBottom(OpSubtract);

        // open shutters
        narrator->SetMessage("");
        CORO_YIELD(1);
        float dt;
        while((dt=Game::GetCube(1)->OpenShutters(&Background)) >= 0)
        {
            CORO_YIELD(dt);
        }
        // initialize two token views
        firstToken = new(tokenViewBuffer[0]) TokenView(Game::GetCube(1), puzzle->GetToken(0), true);
        firstToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_LEFT | TokenView::BIT_TOP);
        CORO_YIELD(0.25f);
        while((dt=Game::GetCube(2)->OpenShutters(&Background)) >= 0)
        {
            CORO_YIELD(dt);
        }
        secondToken = new(tokenViewBuffer[1]) TokenView(Game::GetCube(2), puzzle->GetToken(1), true);
        secondToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_RIGHT | TokenView::BIT_TOP);
        CORO_YIELD(0.5f);

        // wait for neighbor
        narrator->SetMessage("These are called Keys.");
        CORO_YIELD(3);
        narrator->SetMessage("Notice how each Key\nhas a number.");
        CORO_YIELD(3);

        narrator->SetMessage("Connect two Keys\nto combine them.");
        Game::GetInstance().neighborEventHandler = &connectTwoCubesHorizontalEventHandler;

        while(firstToken->token->current == firstToken->token)
        {
            CORO_YIELD(0);
        }

        Game::GetInstance().neighborEventHandler = NULL;

        // flourish 1
        CORO_YIELD(0.5f);
        narrator->SetMessage("Awesome!", NarratorView::EmoteYay);
        CORO_YIELD(3);
        narrator->SetMessage("The combined Key is\n1+2=3!");
        CORO_YIELD(3);

        // now show top-down
        narrator->SetMessage("Try connecting Keys\nTop-to-Bottom...");
        firstToken->Lock();
        secondToken->Lock();
        {
            TokenGroup *grp = (TokenGroup*)firstToken->token->current;
            delete grp;
            firstToken->token->PopGroup();
            secondToken->token->PopGroup();
            firstToken->DidGroupDisconnect();
            secondToken->DidGroupDisconnect();
            firstToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_LEFT | TokenView::BIT_RIGHT);
            secondToken->SetHideMode(TokenView::BIT_TOP | TokenView::BIT_LEFT | TokenView::BIT_RIGHT);
        }
        firstToken->Unlock();
        secondToken->Unlock();

        Game::GetInstance().neighborEventHandler = &connectTwoCubesVerticalEventHandler;

        while(firstToken->token->current == firstToken->token)
        {
            CORO_YIELD(0);
        }

        Game::GetInstance().neighborEventHandler = NULL;

        // flourish 2
        CORO_YIELD(0.5f);
        narrator->SetMessage("Good job!", NarratorView::EmoteYay);
        CORO_YIELD(3);
        narrator->SetMessage("See how this\ncombination equals 2-1=1.");
        CORO_YIELD(3);

        // mix up
        narrator->SetMessage("Okay, let's mix them up a little...", NarratorView::EmoteMix01);
        firstToken->Lock();
        secondToken->Lock();
        {
            TokenGroup *grp = (TokenGroup*)firstToken->token->current;
            delete grp;
            firstToken->token->PopGroup();
            secondToken->token->PopGroup();
            firstToken->DidGroupDisconnect();
            secondToken->DidGroupDisconnect();
            firstToken->SetHideMode(0);
            secondToken->SetHideMode(0);
            firstToken->HideOps();
            secondToken->HideOps();
        }
        firstToken->Unlock();
        secondToken->Unlock();


        // press your luck flourish
        {
            AudioPlayer::PlaySfx(sfx_Tutorial_Mix_Nums);
            rememberedTimeout = period;
            rememberedCubeId = 0;
            remembered_t=0;
            while(remembered_t<3.0f) {
                remembered_t += mGame->dt;
                rememberedTimeout -= mGame->dt;
                while (rememberedTimeout < 0) {
                    (rememberedCubeId==0?firstToken:secondToken)->PaintRandomNumeral();
                    narrator->SetEmote(rememberedCubeId==0? NarratorView::EmoteMix01 : NarratorView::EmoteMix02);
                    rememberedCubeId = (rememberedCubeId+1) % 2;
                    rememberedTimeout += period;
                }
                CORO_YIELD(0);
            }
            firstToken->token->val = 2;
            firstToken->token->SetOpRight(OpMultiply);
            secondToken->token->val = 3;
            secondToken->token->SetOpBottom(OpDivide);
            // thread in the real numbers
            rememberedFinishCountdown = 2;
            while(rememberedFinishCountdown > 0) {
                CORO_YIELD(0);
                rememberedTimeout -= mGame->dt;
                while(rememberedFinishCountdown > 0 && rememberedTimeout < 0) {
                    --rememberedFinishCountdown;
                    (rememberedCubeId==1?firstToken:secondToken)->ResetNumeral();
                    rememberedCubeId++;
                    rememberedTimeout += period;
                }
            }
        }

        // can you make 42?
        narrator->SetMessage("Can you build the number 6?");

        mGame->neighborEventHandler = &makeSixEventHandler;


        while(firstToken->token->current->GetValue() != Fraction(6))
        {
            CORO_YIELD(0);
        }
        AudioPlayer::PlaySfx(sfx_Tutorial_Correct);
        AudioPlayer::PlaySfx(sfx_Tutorial_Oops, false);
        Game::ClearCubeEventHandlers();
        mGame->neighborEventHandler = NULL;
        CORO_YIELD(0.5f);
        narrator->SetMessage("Radical!", NarratorView::EmoteYay);
        CORO_YIELD(3.5f);

        // close shutters
        narrator->SetMessage("");
        CORO_YIELD(1);
        secondToken->SetCube(NULL);
        //Game::GetCube(2)->SetView(NULL);
        while((remembered_t = Game::GetCube(2)->CloseShutters(&Background)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        new(blankViewBuffer[2]) BlankView(Game::GetCube(2), NULL);

        //Game::GetCube(1)->SetView(NULL);
        firstToken->SetCube(NULL);
        while((remembered_t = Game::GetCube(1)->CloseShutters(&Background)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        new(blankViewBuffer[1]) BlankView(Game::GetCube(1), NULL);

        CORO_YIELD(1);
        narrator->SetMessage("Keep combining to build even more numbers!", NarratorView::EmoteYay);
        CORO_YIELD(2);

        Game::GetCube(1)->SetView(NULL);
        while((remembered_t = Game::GetCube(1)->OpenShutters(&Tutorial_Groups)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        new(blankViewBuffer[1]) BlankView(Game::GetCube(1), &Tutorial_Groups);

        CORO_YIELD(5);
        narrator->SetMessage("");
        CORO_YIELD(1);
        Game::GetCube(1)->SetView(NULL);
        while((remembered_t = Game::GetCube(1)->CloseShutters(&Tutorial_Groups)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        new(blankViewBuffer[1]) BlankView(Game::GetCube(1), NULL);

        CORO_YIELD(1);
        narrator->SetMessage("If you get stuck, you can shake for a hint.");
        puzzle->target = (TokenGroup*)firstToken->token->current;
        firstToken->token->PopGroup();
        secondToken->token->PopGroup();
        firstToken->DidGroupDisconnect();
        secondToken->DidGroupDisconnect();
        CORO_YIELD(2);

        //Game::GetCube(1)->SetView(NULL);
        firstToken->SetCube(NULL);
        while((remembered_t = Game::GetCube(1)->OpenShutters(&Background)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        firstToken->SetCube(Game::GetCube(1));
        CORO_YIELD(0.1f);

        //Game::GetCube(2)->SetView(NULL);
        secondToken->SetCube(NULL);
        while(( remembered_t = Game::GetCube(2)->OpenShutters(&Background)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        secondToken->SetCube(Game::GetCube(2));
        CORO_YIELD(1);

        narrator->SetMessage("Try it out!  Shake one!");
        Game::GetCube(1)->AddEventHandler(&waitForShakeEventHandler[0]);
        Game::GetCube(2)->AddEventHandler(&waitForShakeEventHandler[1]);
        while(!(waitForShakeEventHandler[0].DidShake()||waitForShakeEventHandler[1].DidShake())) {
            CORO_YIELD(0);
        }
        Game::ClearCubeEventHandlers();
        CORO_YIELD(0.5f);
        narrator->SetMessage("Nice!", NarratorView::EmoteYay);
        CORO_YIELD(3);
        narrator->SetMessage("Careful! You only get a few hints!");
        CORO_YIELD(3);
        narrator->SetMessage("If you forget the target, press the screen.");
        CORO_YIELD(2);
        narrator->SetMessage("Try it out!  Press one!");

        Game::GetCube(1)->AddEventHandler(&waitForTouchEventHandler[0]);
        Game::GetCube(2)->AddEventHandler(&waitForTouchEventHandler[1]);
        while(!(waitForTouchEventHandler[0].DidTouch()||waitForTouchEventHandler[1].DidTouch())) {
            CORO_YIELD(0);
        }
        CORO_YIELD(0.5f);
        narrator->SetMessage("Great!", NarratorView::EmoteYay);
        CORO_YIELD(3);
        firstToken->token->GetPuzzle()->target = NULL;

        Game::GetCube(2)->SetView(NULL);
        while((remembered_t = Game::GetCube(2)->CloseShutters(&Background)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        new(blankViewBuffer[2]) BlankView(Game::GetCube(2), NULL);

        Game::GetCube(1)->SetView(NULL);
        while((remembered_t = Game::GetCube(1)->CloseShutters(&Background)) >= 0)
        {
            CORO_YIELD(remembered_t);
        }
        new(blankViewBuffer[1]) BlankView(Game::GetCube(1), NULL);


        // transition out narrator
        narrator->SetMessage("Let's try it for real, now!", NarratorView::EmoteWave);
        CORO_YIELD(3);
        narrator->SetMessage("Remember, you need to use every Key!");
        CORO_YIELD(3);
        narrator->SetMessage("Press-and-hold a cube to access the main menu.");
        CORO_YIELD(3);
        narrator->SetMessage("Good luck!", NarratorView::EmoteWave);
        CORO_YIELD(3);

        narrator->SetMessage("");

        AudioPlayer::PlayShutterClose();
        Game::GetCube(0)->SetView(NULL);
        for(remembered_t=0; remembered_t<kTransitionDuration; remembered_t+=mGame->dt) {
            narrator->SetTransitionAmount(1.0f-remembered_t/kTransitionDuration);
            CORO_YIELD(0);
        }
        new(blankViewBuffer[0]) BlankView(Game::GetCube(0), NULL);
        CORO_YIELD(0.5);

        mGame->saveData.CompleteTutorial();
        if (mGame->currentPuzzle == NULL) {
#if !DISABLE_CHAPTERS
            if (!mGame->saveData.AllChaptersSolved()) {
                mGame->currentPuzzle = mGame->saveData.FindNextPuzzle();
            } else {
                mGame->currentPuzzle = Database::GetPuzzleInChapter(0, 0);
            }
#endif // #if !DISABLE_CHAPTERS
        }

        mGame->sceneMgr.QueueTransition("Next");

        delete firstToken;
        delete secondToken;
        delete puzzle->target;
        delete puzzle;

        CORO_END;

        return -1;

    }

    void OnNeighborAdd(TotalsCube *c, Cube::Side s, TotalsCube *nc, Cube::Side ns) {

        // used to make sure token views attached to cubes.
        // now this function is only called from one place (makesixeventhandler)
        // and we know that if its cube 1 and 2 they have token views

        if(s != ((ns+2)%4))
        {
            //opposing faces check
            return;
        }

        if (s == SIDE_LEFT || s == SIDE_TOP) {
            OnNeighborAdd(nc, ns, c, s);
            return;
        }

        if(!((c->id() == 1 || c->id() == 2) && (nc->id() == 1 || nc->id() == 2)))
        {
            //cubes 1 and 2 check
            return;
        }


        TokenView *v = (TokenView*)c->GetView();
        TokenView *nv = (TokenView*)nc->GetView();

        Token *t = v->token;
        Token *nt = nv->token;
        // resolve solution
        if (t->current != nt->current) {
            TokenGroup *grp = TokenGroup::Connect(t, kSideToUnit[s], nt);
            if (grp != NULL) {
                v->WillJoinGroup();
                nv->WillJoinGroup();
                grp->SetCurrent(grp);
                v->DidJoinGroup();
                nv->DidJoinGroup();
            }
        }
        //AudioPlayer::PlayNeighborAdd();
    }

    void OnNeighborRemove(TotalsCube *c, Cube::Side s, TotalsCube *nc, Cube::Side ns) {

        // validate args
        if(!((c->id() == 1 || c->id() == 2) && (nc->id() == 1 || nc->id() == 2)))
        {
            //cubes 1 and 2 check
            return;
        }


        Token *t = firstToken->token;
        Token *nt = secondToken->token;
        // resolve soln
        if (t->current == NULL || nt->current == NULL || t->current != nt->current) {
            return;
        }
        TokenGroup *grp = (TokenGroup*)t->current;
        t->PopGroup();
        nt->PopGroup();
        firstToken->DidGroupDisconnect();
        secondToken->DidGroupDisconnect();
        delete grp;
        //Jukebox.PlayNeighborRemove();
    }

    void OnTick (float dt) {
        UPDATE_CORO(Coroutine, dt);
        Game::UpdateCubeViews(dt);
    }

    void OnPaint (bool canvasDirty) {
        if (canvasDirty) { Game::PaintCubeViews(); }
    }

    void OnDispose () {
        Game::ClearCubeEventHandlers();
        Game::ClearCubeViews();
    }
};
}

