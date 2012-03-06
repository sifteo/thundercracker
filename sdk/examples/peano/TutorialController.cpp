#include "config.h"
#include "TutorialController.h"
#include "AudioPlayer.h"
#include "Game.h"
#include "NarratorView.h"
#include "Puzzle.h"
#include "assets.gen.h"
#include "TokenView.h"
#include "BlankView.h"
#include "Token.h"
#include "TokenGroup.h"

namespace TotalsGame
{
namespace TutorialController
{

void OnNeighborAdd(TotalsCube *c, Cube::Side s, TotalsCube *nc, Cube::Side ns);
void OnNeighborRemove(TotalsCube *c, Cube::Side s, TotalsCube *nc, Cube::Side ns);

class ConnectTwoCubesHorizontalEventHandler: public Game::NeighborEventHandler
{
public:
    void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
};



class ConnectTwoCubesVerticalEventHandler: public Game::NeighborEventHandler
{
public:
    void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
};


class MakeSixEventHandler: public Game::NeighborEventHandler
{
public:
    void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
    void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
};


class WaitForShakeEventHandler: public TotalsCube::EventHandler
{
    bool shook;
public:
    WaitForShakeEventHandler();
    void OnCubeShake(TotalsCube *cube);
    bool DidShake();
};

class WaitForTouchEventHanlder: public TotalsCube::EventHandler
{
    bool touched;
public:
    WaitForTouchEventHanlder();
    void OnCubeTouch(TotalsCube *cube, bool _touched);
    bool DidTouch();
};

float remembered_t;
Puzzle *puzzle;
TokenView *firstToken, *secondToken;
float rememberedTimeout;
int rememberedCubeId;
int rememberedFinishCountdown ;

NarratorView *narrator;

ConnectTwoCubesHorizontalEventHandler connectTwoCubesHorizontalEventHandler;
ConnectTwoCubesVerticalEventHandler connectTwoCubesVerticalEventHandler;
MakeSixEventHandler makeSixEventHandler;
WaitForShakeEventHandler waitForShakeEventHandler[2];
WaitForTouchEventHanlder waitForTouchEventHandler[2];




    /* TODO lost and found
     mGame.CubeSet.LostCubeEvent += OnLostCube;
     mGame.CubeSet.NewCubeEvent += OnNewCube;
     */

/*
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




Game::GameState Run() {
    const float kTransitionDuration = 0.2f;
    static char narratorViewBuffer[sizeof(NarratorView)];
    static char blankViewBuffer[NUM_CUBES][sizeof(BlankView)];
    static char tokenViewBuffer[2][sizeof(TokenView)];

    const float period = 0.05f;

    PLAY_MUSIC(sfx_PeanosVaultMenu);
    narrator = NULL;
    puzzle = NULL;
    firstToken = NULL;
    secondToken = NULL;

    // initialize narrator
    narrator = new(narratorViewBuffer) NarratorView(&Game::cubes[0]);

    // initial blanks
    for(int i = 1; i < NUM_CUBES; i++)
    {
        new(blankViewBuffer[i]) BlankView(&Game::cubes[i], NULL);
    }
    Game::Wait(0);

    Game::Wait(0.5f);


    AudioPlayer::PlayShutterOpen();
    for(remembered_t=0; remembered_t<kTransitionDuration; remembered_t+=Game::dt) {
        narrator->SetTransitionAmount(remembered_t/kTransitionDuration);
        Game::Wait(0);
    }
    narrator->SetTransitionAmount(-1);

    // wait a bit
    Game::Wait(0.5f);
    narrator->SetMessage("Hi there! I'm Peano!", NarratorView::EmoteWave);
    Game::Wait(3);
    narrator->SetMessage("We're going to learn\nto solve secret codes!");
    Game::Wait(3);
    narrator->SetMessage("These codes let you\ninto the treasure vault!", NarratorView::EmoteYay);
    Game::Wait(3);

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
    Game::Wait(1);
    Game::cubes[1].OpenShuttersSync(&Background);
    // initialize two token views
    firstToken = new(tokenViewBuffer[0]) TokenView(&Game::cubes[1], puzzle->GetToken(0), true);
    firstToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_LEFT | TokenView::BIT_TOP);
    Game::Wait(0.25f);
    Game::cubes[2].OpenShuttersSync(&Background);
    secondToken = new(tokenViewBuffer[1]) TokenView(&Game::cubes[2], puzzle->GetToken(1), true);
    secondToken->SetHideMode(TokenView::BIT_BOTTOM | TokenView::BIT_RIGHT | TokenView::BIT_TOP);
    Game::Wait(0.5f);

    // wait for neighbor
    narrator->SetMessage("These are called Keys.");
    Game::Wait(3);
    narrator->SetMessage("Notice how each Key\nhas a number.");
    Game::Wait(3);

    narrator->SetMessage("Connect two Keys\nto combine them.");
    Game::neighborEventHandler = &connectTwoCubesHorizontalEventHandler;

    while(firstToken->token->current == firstToken->token)
    {
        Game::Wait(0);
    }

    Game::neighborEventHandler = NULL;

    // flourish 1
    Game::Wait(0.5f);
    narrator->SetMessage("Awesome!", NarratorView::EmoteYay);
    Game::Wait(3);
    narrator->SetMessage("The combined Key is\n1+2=3!");
    Game::Wait(3);

    // now show top-down
    narrator->SetMessage("Try connecting Keys\nTop-to-Bottom...");
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

    Game::neighborEventHandler = &connectTwoCubesVerticalEventHandler;

    while(firstToken->token->current == firstToken->token)
    {
        Game::Wait(0);
    }

    Game::neighborEventHandler = NULL;

    // flourish 2
    Game::Wait(0.5f);
    narrator->SetMessage("Good job!", NarratorView::EmoteYay);
    Game::Wait(3);
    narrator->SetMessage("See how this\ncombination equals 2-1=1.");
    Game::Wait(3);

    // mix up
    narrator->SetMessage("Okay, let's mix them up a little...", NarratorView::EmoteMix01);
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

    //set window to bottom half of screen so we can animate peano
    //while text window is open above
    System::paintSync();
    narrator->GetCube()->backgroundLayer.set();
    narrator->GetCube()->backgroundLayer.clear();
    narrator->GetCube()->backgroundLayer.setWindow(64,64);


    // press your luck flourish
    {
        PLAY_SFX(sfx_Tutorial_Mix_Nums);
        rememberedTimeout = period;
        rememberedCubeId = 0;
        remembered_t=0;
        while(remembered_t<3.0f) {
            remembered_t += Game::dt;
            rememberedTimeout -= Game::dt;
            while (rememberedTimeout < 0) {
                (rememberedCubeId==0?firstToken:secondToken)->PaintRandomNumeral();
                //narrator->SetEmote(rememberedCubeId==0? NarratorView::EmoteMix01 : NarratorView::EmoteMix02);
                rememberedCubeId = (rememberedCubeId+1) % 2;
                rememberedTimeout += period;
            }

            narrator->GetCube()->Image(rememberedCubeId?&Narrator_Mix02:&Narrator_Mix01, Vec2(0, 0), Vec2(0,2), Vec2(16,8));
            firstToken->PaintNow();
            secondToken->PaintNow();
            System::paintSync();
            Game::UpdateDt();
        }

        narrator->GetCube()->backgroundLayer.setWindow(0,128);
        narrator->Paint();
        System::paint();

        firstToken->token->val = 2;
        firstToken->token->SetOpRight(OpMultiply);
        secondToken->token->val = 3;
        secondToken->token->SetOpBottom(OpDivide);
        // thread in the real numbers
        rememberedFinishCountdown = 2;
        while(rememberedFinishCountdown > 0) {
            Game::Wait(0);
            rememberedTimeout -= Game::dt;
            while(rememberedFinishCountdown > 0 && rememberedTimeout < 0) {
                --rememberedFinishCountdown;
                (rememberedCubeId==1?firstToken:secondToken)->ResetNumeral();
                rememberedCubeId++;
                rememberedTimeout += period;
            }
        }
    }

    // can you make 42?
    narrator->SetMessage("Can you build\nthe number 6?");

    Game::neighborEventHandler = &makeSixEventHandler;


    while(firstToken->token->current->GetValue() != Fraction(6))
    {
        Game::Wait(0);
    }
    PLAY_SFX(sfx_Tutorial_Correct);
    PLAY_SFX2(sfx_Tutorial_Oops, false);
    Game::ClearCubeEventHandlers();
    Game::neighborEventHandler = NULL;
    Game::Wait(0.5f);
    narrator->SetMessage("Radical!", NarratorView::EmoteYay);
    Game::Wait(3.5f);

    // close shutters
    narrator->SetMessage("");
    Game::Wait(1);
    secondToken->SetCube(NULL);
    //Game::cubes[2]->SetView(NULL);
    Game::cubes[2].foregroundLayer.Clear();
    Game::cubes[2].foregroundLayer.Flush();
    Game::cubes[2].CloseShuttersSync(&Background);
    new(blankViewBuffer[2]) BlankView(&Game::cubes[2], NULL);

    //Game::cubes[1]->SetView(NULL);
    firstToken->SetCube(NULL);
    Game::cubes[1].foregroundLayer.Clear();
    Game::cubes[1].foregroundLayer.Flush();
    Game::cubes[1].CloseShuttersSync(&Background);
    new(blankViewBuffer[1]) BlankView(&Game::cubes[1], NULL);

    Game::Wait(1);
    narrator->SetMessage("Keep combining to build\neven more numbers!", NarratorView::EmoteYay);
    Game::Wait(2);

    Game::cubes[1].SetView(NULL);
    Game::cubes[1].OpenShuttersSync(&Tutorial_Groups);
    new(blankViewBuffer[1]) BlankView(&Game::cubes[1], &Tutorial_Groups);

    Game::Wait(5);
    narrator->SetMessage("");
    Game::Wait(1);
    Game::cubes[1].SetView(NULL);
    Game::cubes[1].CloseShuttersSync(&Tutorial_Groups);
    new(blankViewBuffer[1]) BlankView(&Game::cubes[1], NULL);

    Game::Wait(1);
    narrator->SetMessage("If you get stuck,\nyou can shake\nfor a hint.");
    puzzle->target = (TokenGroup*)firstToken->token->current;
    firstToken->token->PopGroup();
    secondToken->token->PopGroup();
    firstToken->DidGroupDisconnect();
    secondToken->DidGroupDisconnect();
    Game::Wait(2);

    //Game::cubes[1]->SetView(NULL);
    Game::cubes[1].OpenShuttersSync(&Background);
    firstToken->SetCube(&Game::cubes[1]);
    firstToken->PaintNow();
    Game::Wait(0.1f);

    //Game::cubes[2]->SetView(NULL);
    secondToken->SetCube(NULL);
    Game::cubes[2].OpenShuttersSync(&Background);
    secondToken->SetCube(&Game::cubes[2]);
    secondToken->PaintNow();
    Game::Wait(1);

    narrator->SetMessage("Try it out!  Shake one!");
    Game::cubes[1].AddEventHandler(&waitForShakeEventHandler[0]);
    Game::cubes[2].AddEventHandler(&waitForShakeEventHandler[1]);
    while(!(waitForShakeEventHandler[0].DidShake()||waitForShakeEventHandler[1].DidShake())) {
        Game::Wait(0);
    }
    Game::ClearCubeEventHandlers();
    Game::Wait(0.5f);
    narrator->SetMessage("Nice!", NarratorView::EmoteYay);
    Game::Wait(3);
    narrator->SetMessage("Careful!\nYou only get a few hints!");
    Game::Wait(3);
    narrator->SetMessage("If you forget\nthe target,\npress the screen.");
    Game::Wait(2);
    narrator->SetMessage("Try it out!  Press one!");

    Game::cubes[1].AddEventHandler(&waitForTouchEventHandler[0]);
    Game::cubes[2].AddEventHandler(&waitForTouchEventHandler[1]);
    while(!(waitForTouchEventHandler[0].DidTouch()||waitForTouchEventHandler[1].DidTouch())) {
        Game::Wait(0);
    }

    Game::Wait(0.5f);
    narrator->SetMessage("Great!", NarratorView::EmoteYay);
    Game::Wait(3);
    firstToken->token->GetPuzzle()->target = NULL;

    Game::cubes[2].HideSprites();
    Game::cubes[2].foregroundLayer.Clear();
    Game::cubes[2].foregroundLayer.Flush();
    Game::cubes[2].SetView(NULL);
    Game::cubes[2].CloseShuttersSync(&Background);
    new(blankViewBuffer[2]) BlankView(&Game::cubes[2], NULL);

    Game::cubes[1].HideSprites();
    Game::cubes[1].foregroundLayer.Clear();
    Game::cubes[1].foregroundLayer.Flush();
    Game::cubes[1].SetView(NULL);
    Game::cubes[1].CloseShuttersSync(&Background);
    new(blankViewBuffer[1]) BlankView(&Game::cubes[1], NULL);


    // transition out narrator
    narrator->SetMessage("Let's try it\nfor real, now!", NarratorView::EmoteWave);
    Game::Wait(3);
    narrator->SetMessage("Remember, you need\nto use every Key!");
    Game::Wait(3);
    narrator->SetMessage("Press-and-hold a cube\nto access the main menu.");
    Game::Wait(3);
    narrator->SetMessage("Good luck!", NarratorView::EmoteWave);
    Game::Wait(3);

    narrator->SetMessage("");
    narrator->GetCube()->UpdateTextOverlay();

    AudioPlayer::PlayShutterClose();
    Game::cubes[0].SetView(NULL);
    for(remembered_t=0; remembered_t<kTransitionDuration; remembered_t+=Game::dt) {
        narrator->SetTransitionAmount(1.0f-remembered_t/kTransitionDuration);
        Game::Wait(0);
    }
    new(blankViewBuffer[0]) BlankView(&Game::cubes[0], NULL);
    Game::Wait(0.5);

    delete firstToken;
    delete secondToken;
    delete puzzle->target;
    delete puzzle;

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    //free up our tokens
    Token::ResetAllocationPool();
    TokenGroup::ResetAllocationPool();

    Game::saveData.CompleteTutorial();
    if (Game::currentPuzzle == NULL) {
#if !DISABLE_CHAPTERS
        if (!Game::saveData.AllChaptersSolved()) {
            Game::currentPuzzle = Game::saveData.FindNextPuzzle();
        } else {
            Game::currentPuzzle = Database::GetPuzzleInChapter(0, 0);
        }
#endif // #if !DISABLE_CHAPTERS
    }

    return Game::GameState_Interstitial;


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





    void ConnectTwoCubesHorizontalEventHandler::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        if ((s0 == SIDE_RIGHT && s1 == SIDE_LEFT && c1 == secondToken->GetCube()->id())
            ||(s0 == SIDE_LEFT && s1 == SIDE_RIGHT && c0 == secondToken->GetCube()->id()))
        {
            Game::neighborEventHandler = NULL;
            TokenGroup *grp = TokenGroup::Connect(firstToken->token, kSideToUnit[SIDE_RIGHT], secondToken->token);
            if (grp != NULL)
            {
                firstToken->WillJoinGroup();
                secondToken->WillJoinGroup();
                grp->SetCurrent(grp);
                firstToken->DidJoinGroup();
                secondToken->DidJoinGroup();
            }
            PLAY_SFX(sfx_Tutorial_Correct);
        }
    }





    void ConnectTwoCubesVerticalEventHandler::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        if ((s0 == SIDE_TOP && s1 == SIDE_BOTTOM && c1 == secondToken->GetCube()->id())
            ||(s0 == SIDE_BOTTOM && s1 == SIDE_TOP && c0 == secondToken->GetCube()->id()))
        {
            Game::neighborEventHandler = NULL;
            TokenGroup *grp = TokenGroup::Connect(firstToken->token, kSideToUnit[SIDE_TOP], secondToken->token);
            if (grp != NULL)
            {
                firstToken->WillJoinGroup();
                secondToken->WillJoinGroup();
                grp->SetCurrent(grp);
                firstToken->DidJoinGroup();
                secondToken->DidJoinGroup();
            }
            PLAY_SFX(sfx_Tutorial_Correct);
        }
    }




    void MakeSixEventHandler::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        TutorialController::OnNeighborAdd(&Game::cubes[c0], s0, &Game::cubes[c1], s1);
        if (!firstToken->token->current->GetValue() == Fraction(6)) {
            PLAY_SFX2(sfx_Tutorial_Oops, false);
            narrator->SetMessage("Oops, let's try again...", NarratorView::EmoteSad);
        }
    }

    void MakeSixEventHandler::OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        TutorialController::OnNeighborRemove(&Game::cubes[c0],s0, &Game::cubes[c1], s1);
        if (firstToken->token->current == firstToken->token) {
            narrator->SetMessage("Can you build the number 6?");
        }
    }



    WaitForShakeEventHandler::WaitForShakeEventHandler()
    {
        shook = false;
    }

    void WaitForShakeEventHandler::OnCubeShake(TotalsCube *cube)
    {
        shook = true;
    }

    bool WaitForShakeEventHandler::DidShake()
    {
        return shook;
    }



    WaitForTouchEventHanlder::WaitForTouchEventHanlder()
    {
        touched = false;
    }

    void WaitForTouchEventHanlder::OnCubeTouch(TotalsCube *cube, bool _touched)
    {
        touched = touched | _touched;
    }

    bool WaitForTouchEventHanlder::DidTouch()
    {
        return touched;
    }






}

}
