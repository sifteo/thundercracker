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

    class ConnectTwoCubesEventHandler: public Game::NeighborEventHandler
    {
        TutorialController *owner;
    public:
        ConnectTwoCubesEventHandler(TutorialController *_owner)
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
    ConnectTwoCubesEventHandler connectTwoCubesEventHandler;



    CORO_PARAMS;
    float remembered_t;
    Puzzle *puzzle;
    TokenView *firstToken, *secondToken;

    NarratorView *narrator;
    Game *mGame;
public:
    TutorialController (Game *game):
        connectTwoCubesEventHandler(this)
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

        CORO_BEGIN;

        // initialize narrator
        narrator = new NarratorView(Game::GetCube(0));

        // initial blanks
        for(int i = 1; i < Game::NUMBER_OF_CUBES; i++)
        {
            new BlankView(Game::GetCube(i), NULL);
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
        firstToken = new TokenView(Game::GetCube(1), puzzle->GetToken(0), true);
        firstToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_LEFT | TokenView::BIT_TOP);
        CORO_YIELD(0.25f);
        while((dt=Game::GetCube(2)->OpenShutters(&Background)) >= 0)
        {
            CORO_YIELD(dt);
        }
        secondToken = new TokenView(Game::GetCube(2), puzzle->GetToken(1), true);
        secondToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_RIGHT | TokenView::BIT_TOP);
        CORO_YIELD(0.5f);

        // wait for neighbor
        narrator->SetMessage("These are called Keys.");
        CORO_YIELD(3);
        narrator->SetMessage("Notice how each Key\nhas a number.");
        CORO_YIELD(3);

        narrator->SetMessage("Connect two Keys\nto combine them.");
        Game::GetInstance().neighborEventHandler = &connectTwoCubesEventHandler;

        while(firstToken->token->current == firstToken->token)
        {
            CORO_YIELD(0);
        }

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
#if 0
// wait for neighbor
firstToken.Cube.NeighborAddEvent += (c, side, neighbor, neighborSide) => {
        if (side == Cube.Side.TOP && neighborSide == Cube.Side.BOTTOM && neighbor == secondToken.Cube) {
        firstToken.Cube.ClearEvents();
var grp = TokenGroup.Connect(secondToken.token, Int2.Down, firstToken.token);
if (grp != null) {
    foreach(var token in grp.Tokens) { token.GetView().WillJoinGroup(); }
    grp.SetCurrent(grp);
    foreach(var token in grp.Tokens) { token.GetView().DidJoinGroup(); }
}
firstToken.Cube.ClearEvents();
Jukebox.Sfx("PV_tutorial_correct");
}
};
while(firstToken.token.current == firstToken.token) { yield return 0; }

// flourish 2
yield return 0.5f;
narrator.SetMessage("Good job!", "yay");
yield return 3f;
narrator.SetMessage("See how this combination equals 2-1=1.");
yield return 3f;

// mix up
narrator.SetMessage("Okay, let's mix them up a little...", "mix01");
firstToken.Lock();
secondToken.Lock();
{
var grp = firstToken.token.current as TokenGroup;
firstToken.token.PopGroup();
secondToken.token.PopGroup();
foreach(var token in grp.Tokens) { token.GetView().DidGroupDisconnect(); }
firstToken.SetHideMode(0);
secondToken.SetHideMode(0);
firstToken.HideOps();
secondToken.HideOps();
}
firstToken.Unlock();
secondToken.Unlock();
// press your luck flourish
{
Jukebox.Sfx("PV_tutorial_mix_nums");
var period = 0.05f;
var timeout = period;
int cubeId = 0;
var t=0f;
while(t<3f) {
    t += mGame.dt;
    timeout -= mGame.dt;
    while (timeout < 0f) {
        (cubeId==0?firstToken:secondToken).PaintRandomNumeral();
        narrator.SetEmote("mix0" + (cubeId+1));
        cubeId = (cubeId+1) % 2;
        timeout += period;
    }
    yield return 0f;
}
firstToken.token.val = 2;
firstToken.token.OpRight = Op.Multiply;
secondToken.token.val = 3;
secondToken.token.OpBottom = Op.Divide;
// thread in the real numbers
int finishCountdown = 2;
while(finishCountdown > 0) {
    yield return 0f;
    timeout -= mGame.dt;
    while(finishCountdown > 0 && timeout < 0f) {
        --finishCountdown;
        (cubeId==1?firstToken:secondToken).ResetNumeral();
        cubeId++;
        timeout += period;
    }
}
}

// can you make 42?
narrator.SetMessage("Can you build the number 6?");

mGame.CubeSet.NeighborAddEvent += delegate(Cube c1, Cube.Side side1, Cube c2, Cube.Side side2) {
        OnNeighborAdd(c1, side1, c2, side2);
if (!firstToken.token.current.Value.Equals(new Fraction(6))) {
    Jukebox.Sfx("PV_tutorial_oops", false);
    narrator.SetMessage("Oops, let's try again...", "sad");
}
};
mGame.CubeSet.NeighborRemoveEvent += delegate(Cube c1, Cube.Side side1, Cube c2, Cube.Side side2) {
        OnNeighborRemove(c1, side1, c2, side2);
if (firstToken.token.current == firstToken.token) {
    narrator.SetMessage("Can you build the number 6?");
}
};
while(!firstToken.token.current.Value.Equals(new Fraction(6))) {
    yield return 0;
}
Jukebox.Sfx("PV_tutorial_correct");Jukebox.Sfx("PV_tutorial_oops", false);
mGame.CubeSet.ClearEvents();
yield return 0.5f;
narrator.SetMessage("Radical!", "yay");
yield return 3.5f;

// close shutters
narrator.SetMessage("");
yield return 1f;
foreach(var dt in mGame.CubeSet[2].CloseShutters()) { yield return dt; }
new BlankView(secondToken.Cube);
foreach(var dt in mGame.CubeSet[1].CloseShutters()) { yield return dt; }
new BlankView(firstToken.Cube);
yield return 1f;
narrator.SetMessage("Keep combining to build even more numbers!", "yay");
yield return 2f;
foreach(var dt in mGame.CubeSet[1].OpenShutters("tutorial_groups")) { yield return dt; }
new BlankView(mGame.CubeSet[1], "tutorial_groups");
yield return 5f;
narrator.SetMessage("");
yield return 1f;
foreach(var dt in mGame.CubeSet[1].CloseShutters()) { yield return dt; }
new BlankView(mGame.CubeSet[1]);
yield return 1f;

narrator.SetMessage("If you get stuck, you can shake for a hint.");
puzzle.target = firstToken.token.current as TokenGroup;
firstToken.token.PopGroup();
secondToken.token.PopGroup();
firstToken.DidGroupDisconnect();
secondToken.DidGroupDisconnect();
yield return 2f;
foreach(var dt in mGame.CubeSet[1].OpenShutters("background")) { yield return dt; }
firstToken.Cube = mGame.CubeSet[1];
yield return 0.1f;
foreach(var dt in mGame.CubeSet[2].OpenShutters("background")) { yield return dt; }
secondToken.Cube = mGame.CubeSet[2];
yield return 1f;

narrator.SetMessage("Try it out!  Shake one!");
while(!(firstToken.Cube.IsShaking || secondToken.Cube.IsShaking)) {
    yield return 0f;
}
yield return 0.5f;
narrator.SetMessage("Nice!", "yay");
yield return 3f;
narrator.SetMessage("Careful! You only get a few hints!");
yield return 3f;
narrator.SetMessage("If you forget the target, press the screen.");
yield return 2f;
narrator.SetMessage("Try it out!  Press one!");
while(!(firstToken.Cube.ButtonIsPressed || secondToken.Cube.ButtonIsPressed)) {
    yield return 0;
}
yield return 0.5f;
narrator.SetMessage("Great!", "yay");
yield return 3f;
firstToken.token.puzzle.target = null;

foreach(var dt in mGame.CubeSet[2].CloseShutters()) { yield return dt; }
new BlankView(secondToken.Cube);
foreach(var dt in mGame.CubeSet[1].CloseShutters()) { yield return dt; }
new BlankView(firstToken.Cube);


// transition out narrator
narrator.SetMessage("Let's try it for real, now!", "wave");
yield return 3f;
narrator.SetMessage("Remember, you need to use every Key!");
yield return 3f;
narrator.SetMessage("Press-and-hold a cube to access the main menu.");
yield return 3f;
narrator.SetMessage("Good luck!", "wave");
yield return 3f;

Jukebox.PlayShutterClose();
for(var t=0f; t<kTransitionDuration; t+=mGame.dt) {
    narrator.SetTransitionAmount(1f-t/kTransitionDuration);
    yield return 0f;
}
new BlankView(narrator.Cube);
yield return 0.5f;

mGame.saveData.CompleteTutorial();
if (mGame.currentPuzzle == null) {
    if (!mGame.saveData.AllChaptersSolved) {
        mGame.currentPuzzle = mGame.saveData.FindNextPuzzle();
    } else {
        mGame.currentPuzzle = mGame.database.Chapters[0].Puzzles[0];
    }
}

#endif

narrator->SetMessage("");
mGame->sceneMgr.QueueTransition("Next");

CORO_END;

return -1;

}

void OnNeighborAdd(TotalsCube *c, Cube::Side s, TotalsCube *nc, Cube::Side ns) {
    /* TODO
    // validate args
    var v = c.GetTokenView();
    var nv = nc.GetTokenView();
    if (v == null || nv == null) { return; }
    if (s != CubeHelper.InvertSide(ns)) { return; }
    if (s == Cube.Side.LEFT || s == Cube.Side.TOP) {
        OnNeighborAdd(nc, ns, c, s);
        return;
    }
    var t = v.token;
    var nt = nv.token;
    // resolve solution
    if (t.current != nt.current) {
        var grp = TokenGroup.Connect(t.current, t, Int2.Side(s), nt.current, nt);
        if (grp != null) {
            foreach(var token in grp.Tokens) { token.GetView().WillJoinGroup(); }
            grp.SetCurrent(grp);
            foreach(var token in grp.Tokens) { token.GetView().DidJoinGroup(); }
        }
    }
    //Jukebox.PlayNeighborAdd();
*/
}

void OnNeighborRemove(TotalsCube *c, Cube::Side s, TotalsCube *nc, Cube::Side ns) {
    /* TODO
    // validate args
    var v = c.userData as TokenView;
    var nv = nc.userData as TokenView;
    if (v == null || nv == null) { return; }
    var t = v.token;
    var nt = nv.token;
    // resolve soln
    if (t.current == null || nt.current == null || t.current != nt.current) {
        return;
    }
    var grp = t.current as TokenGroup;
    t.PopGroup();
    nt.PopGroup();
    foreach(var token in grp.Tokens) { token.GetView().DidGroupDisconnect(); }
    //Jukebox.PlayNeighborRemove();
*/
}

float OnTick (float dt) {
    float ret = Coroutine(dt);
    Game::UpdateCubeViews(dt);
    return ret;
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

