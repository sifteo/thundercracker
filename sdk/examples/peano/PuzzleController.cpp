#include "PuzzleController.h"
#include "assets.gen.h"
#include "PauseHelper.h"
#include "ConfirmationMenu.h"
#include "NarratorView.h"
#include "Skins.h"

namespace TotalsGame {

namespace PuzzleController
{

class EventHandler: public TotalsCube::EventHandler
{
public:
    void OnCubeShake(TotalsCube *cube) {};
    void OnCubeTouch(TotalsCube *cube, bool touching) {};
};
EventHandler eventHandlers[NUM_CUBES];

class NeighborEventHandler: public Game::NeighborEventHandler
{
public:
    void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
    void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
};
NeighborEventHandler neighborEventHandler;

struct RemoveEvent
{
    Cube::ID c0;
    Cube::Side s0;
    Cube::ID c1;
    Cube::Side s1;

    void Set(Cube::ID _c0, Cube::Side _s0, Cube::ID _c1, Cube::Side _s1)
    {
        c0 = _c0;
        s0 = _s0;
        c1 = _c1;
        s1 = _s1;
    }
};
//this really should be enough.  can always disconnect all and start ignoring them.
#define MAX_REMOVE_EVENTS (NUM_CUBES*2)
RemoveEvent removeEvents[MAX_REMOVE_EVENTS];
int numRemoveEvents = 0;

void ProcessRemoveEventBuffer();

Puzzle *puzzle;
bool paused;


void OnSetup ()
{
    numRemoveEvents = 0;
    AudioPlayer::PlayInGameMusic();

    String<10> stringRep;
    int stringRepLength;
    if (Game::IsPlayingRandom())
    {
        int nCubes = 3 + ( NUM_CUBES > 3 ? Random().randrange(NUM_CUBES-3+1) : 0 );
        puzzle = PuzzleHelper::SelectRandomTokens(Game::difficulty, nCubes);
        // hacky reliability :P
        int retries = 0;
        do
        {
            if (retries == 100)
            {
                retries = 0;
                delete puzzle;  //selectrandomtokens does a new
                puzzle = PuzzleHelper::SelectRandomTokens(Game::difficulty, nCubes);
            }
            else
            {
                TokenGroup::ResetAllocationPool();
                retries++;
            }
            puzzle->SelectRandomTarget();
            stringRep.clear();
            puzzle->target->GetValue().ToString(&stringRep);
            stringRepLength = stringRep.size();
        } while(stringRepLength > 4 || puzzle->target->GetValue().IsNan());
    }
    else
    {
        puzzle = Game::currentPuzzle;
        puzzle->ClearGroups();
        puzzle->target->RecomputeValue(); // in case the difficulty changed
    }
    paused = false;
    puzzle->hintsUsed = 0;

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[0].AddEventHandler(&eventHandlers[i]);
    }

    //
}

/* TODO lost and found
  game.CubeSet.LostCubeEvent += OnCubeLost;
         on cube found put a blank view on it
         void OnCubeLost(Cube c) {
         if (!c.IsUnused()) {
         // for now, let's just return to the main menu
         Transition("Quit");
         }
         }
         */
void ShowPuzzleCount()
{
    if(!Game::IsPlayingRandom())
    { // opening remarks
        int count = 1 + puzzle->CountAfterThisInChapterWithCurrentCubeSet();
        if (count > 0)
        {
            NarratorView nv;
            Game::cubes[0].SetView(&nv);
            const float kTransitionTime = 0.2f;
            AudioPlayer::PlayShutterOpen();
            for(float t=0; t<kTransitionTime; t+=Game::dt) {
                nv.SetTransitionAmount(t/kTransitionTime);
                Game::Wait(0);
            }
            nv.SetTransitionAmount(-1);
            Game::Wait(0.5f);
            if (count == 1) {
                nv.SetMessage("1 code to go...", NarratorView::EmoteMix01);

                //set window to bottom half of screen so we can animate peano
                //while text window is open above
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
            } else {
                //static because nv.SetMessage doesnt strcpy, keeps pointer.
                static String<20> msg;
                msg.clear();
                msg << count << " codes to go...";
                nv.SetMessage(msg);
                Game::Wait(2);
            }

            nv.SetMessage("");

            AudioPlayer::PlayShutterClose();
            for(float t=0; t<kTransitionTime; t+=Game::dt) {
                nv.SetTransitionAmount(1-t/kTransitionTime);
                Game::Wait(0);
            }
            nv.SetTransitionAmount(0);
            Game::Wait(0);

            Game::cubes[0].SetView(NULL);

        }
    }
}

//-------------------------------------------------------------------------
// MAIN CORO
//-------------------------------------------------------------------------

Game::GameState Run()
{

    OnSetup();
    ShowPuzzleCount();

    PauseHelper *pauseHelper;

    bool transitioningOut;

    for(int i = puzzle->GetNumTokens(); i < NUM_CUBES; i++)
    {
        Game::cubes[i].DrawVaultDoorsClosed();
    }
    Game::Wait(0.25);

    TokenView tv[NUM_CUBES];
    for(int i = 0; i < puzzle->GetNumTokens(); i++)
    {
        Game::cubes[i].SetView(tv+i);
        tv[i].SetToken(puzzle->GetToken(i));
        Game::cubes[i].OpenShuttersSync(&Skins::GetSkin().background);
        tv[i].PaintNow();
    }

    Game::Wait(0.1f);

    const Skins::Skin &skin = Skins::GetSkin();

    { // flourish in
        Game::Wait(1.1f);
        // show total
        PLAY_SFX(sfx_Tutorial_Mix_Nums);

        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {
            puzzle->GetToken(i)->GetTokenView()->ShowOverlay();
        }
        Game::Wait(3.0f);
        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {
            puzzle->GetToken(i)->GetTokenView()->HideOverlay();
        }
    }

    { // gameplay
        PauseHelper pauseHelper;

        // subscribe to events
        Game::neighborEventHandler = &neighborEventHandler;
        { // game loop
            while(!puzzle->IsComplete()) {
                // most stuff is handled by events and view updaters
                Game::Wait(0);

                // should pause?
                pauseHelper.Update();
                if (pauseHelper.IsTriggered())
                {
                    paused = true;

                    // transition out
                    for(int t=0; t<puzzle->GetNumTokens(); ++t)
                    {
                        Game::cubes[t].HideSprites();
                        Game::cubes[t].foregroundLayer.Clear();
                        Game::cubes[t].foregroundLayer.Flush();
                        Game::cubes[t].SetView(NULL);

                        Game::cubes[t].CloseShuttersSync(&skin.background);
                        Game::cubes[t].DrawVaultDoorsClosed();
                    }

                    // do menu
                    if (ConfirmationMenu::Run("Return to Menu?"))
                    {
                        Game::neighborEventHandler = NULL;
                        Game::ClearCubeEventHandlers();
                        Game::ClearCubeViews();
                        Token::ResetAllocationPool();
                        TokenGroup::ResetAllocationPool();
                        delete puzzle;
                        Game::currentPuzzle = NULL;
                        return Game::GameState_Menu;
                    }


                    // transition back
                    for(int i=0; i<puzzle->GetNumTokens(); ++i)
                    {
                        Game::cubes[i].SetView(NULL);

                        Game::cubes[i].OpenShuttersSync(&skin.background);

                        Game::cubes[i].SetView(puzzle->GetToken(i)->GetTokenView());
                        ((TokenView*)Game::cubes[i].GetView())->PaintNow();
                        Game::Wait(0.1f);
                    }

                    pauseHelper.Reset();
                    paused = false;
                    ProcessRemoveEventBuffer();
                }
            }
        }

    }
    // unsubscribe
    Game::neighborEventHandler = NULL;


    { // flourish out
        Game::Wait(1);
        PLAY_SFX(sfx_Level_Clear);
        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {
            puzzle->GetToken(i)->GetTokenView()->ShowLit();
        }
        Game::Wait(3);
    }

    { // transition out
        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {                      
            TotalsCube *c = puzzle->GetToken(i)->GetTokenView()->GetCube();
            c->HideSprites();
            c->foregroundLayer.Clear();
            c->foregroundLayer.Flush();
            c->SetView(NULL);

            c->CloseShuttersSync(&skin.background_lit);
            Game::Wait(0.1f);
        }
    }

    if (Game::IsPlayingRandom())
    {
        //advance deletes chapter created puzzles
        delete puzzle;
    }

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();
    Token::ResetAllocationPool();
    TokenGroup::ResetAllocationPool();

    return Game::GameState_Advance;
}

//-------------------------------------------------------------------------
// EVENTS
//-------------------------------------------------------------------------
void PuzzleController::NeighborEventHandler::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    if (paused || numRemoveEvents > 0) { return; }

    // validate args
    TokenView *v = (TokenView*)Game::cubes[c0].GetView();
    TokenView *nv = (TokenView*)Game::cubes[c1].GetView();
    if (v == NULL || nv == NULL) { return; }
    if (s0 != ((s1+2)%NUM_SIDES)) { return; }
    if (s0 == SIDE_LEFT || s0 == SIDE_TOP)
    {
        OnNeighborAdd(c1, s1, c0, s0);
        return;
    }
    Token *t = v->token;
    Token *nt = nv->token;
    // resolve solution
    if (t->current != nt->current)
    {
        AudioPlayer::PlayNeighborAdd();
        TokenGroup *grp = TokenGroup::Connect(t->current, t, kSideToUnit[s0], nt->current, nt);
        if (grp != NULL) {
            grp->AlertWillJoinGroup();
            grp->SetCurrent(grp);
            grp->AlertDidJoinGroup();                }
    }
}

void PuzzleController::NeighborEventHandler::OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    if (paused || numRemoveEvents > 0)
    {
        if(numRemoveEvents < MAX_REMOVE_EVENTS)
        {
            removeEvents[numRemoveEvents].Set(c0, s0, c1, s1);
        }
        //overflowing the array will signal to disconnect everything
        //since we can't keep track of any more
        numRemoveEvents++;
        return;

    }


    // validate args
    TokenView *v = (TokenView*)Game::cubes[c0].GetView();
    TokenView *nv = (TokenView*)Game::cubes[c1].GetView();
    if (v == NULL || nv == NULL) { return; }
    Token *t = v->token;
    Token *nt = nv->token;
    // resolve soln
    if (t->current == NULL || nt->current == NULL || t->current != nt->current)
    {
        return;
    }
    AudioPlayer::PlayNeighborRemove();
    TokenGroup *grp = (TokenGroup*)t->current;
    while(t->current->Contains(nt)) {
        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {
            Token *token = puzzle->GetToken(i);
            if (token != t && token->current == t->current) {
                token->PopGroup();
            }
        }
        IExpression *current = t->current;
        t->PopGroup();
        if(current != grp && current->IsTokenGroup())
        {   //save the root delete until after the below alertDidDisconnect
            delete (TokenGroup*)current;
        }
    }
    grp->AlertDidGroupDisconnect();
    delete grp;
}


void ProcessRemoveEventBuffer()
{
    if(numRemoveEvents <= MAX_REMOVE_EVENTS)
    {
        int count = numRemoveEvents;
        numRemoveEvents = 0;    //needed for OnNeighborRemove to actually do work
        for(int i = 0; i < count; i++)
        {
            neighborEventHandler.OnNeighborRemove(removeEvents[i].c0, removeEvents[i].s0, removeEvents[i].c1, removeEvents[i].s1);
        }
    }
    else
    {
        numRemoveEvents = 0;
        //more disconnects occured than we can record.
        //disconnect everything
        for(int i = 0; i < NUM_CUBES; i++)
        {
            for(int j = i+1; j < NUM_CUBES; j++)
            {
                //neighbor remove handler ignores sides
                neighborEventHandler.OnNeighborRemove(i, 0, j, 0);
            }

        }
    }
}

}


}
