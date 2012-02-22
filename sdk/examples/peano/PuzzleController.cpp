#include "PuzzleController.h"
#include "BlankView.h"
#include "assets.gen.h"

namespace TotalsGame {

bool PuzzleController::IsPaused()
{
    return mPaused || game->IsPaused;
}

PuzzleController::PuzzleController(Game *_game) :
    neighborEventHandler(this)
{
    game = _game; mPaused = false;
}

void PuzzleController::OnSetup ()
{
    CORO_RESET;

    AudioPlayer::PlayInGameMusic();
    mTransitioningOut = false;
    String<10> stringRep;
    int stringRepLength;
    if (game->IsPlayingRandom())
    {
        int nCubes = 3 + ( Game::NUMBER_OF_CUBES > 3 ? Random().randrange(Game::NUMBER_OF_CUBES-3+1) : 0 );
        puzzle = PuzzleHelper::SelectRandomTokens(game->difficulty, nCubes);
        // hacky reliability :P
        int retries = 0;
        do
        {
            if (retries == 100)
            {
                retries = 0;
                delete puzzle;  //selectrandomtokens does a new
                puzzle = PuzzleHelper::SelectRandomTokens(game->difficulty, nCubes);
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
        puzzle = game->currentPuzzle;
        puzzle->ClearGroups();
        puzzle->target->RecomputeValue(); // in case the difficulty changed
    }
    mPaused = false;
    puzzle->hintsUsed = 0;

    for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
    {
        Game::GetCube(i)->AddEventHandler(&eventHandler);
    }

    //TODO game.CubeSet.LostCubeEvent += OnCubeLost;
}

/* TODO
        on cube found put a blank view on it
        void OnCubeLost(Cube c) {
        if (!c.IsUnused()) {
        // for now, let's just return to the main menu
        Transition("Quit");
        }
        }
        */
//-------------------------------------------------------------------------
// MAIN CORO
//-------------------------------------------------------------------------

float PuzzleController::TheBigCoroutine(float dt)
{
    static char blankViewBuffer[Game::NUMBER_OF_CUBES][sizeof(BlankView)];
    static char tokenViewBuffer[Game::NUMBER_OF_CUBES][sizeof(TokenView)];

    CORO_BEGIN;

    for(int i = puzzle->GetNumTokens(); i < Game::NUMBER_OF_CUBES; i++)
    {
        new(blankViewBuffer[i]) BlankView(Game::GetCube(i), NULL);
    }
    Game::DrawVaultDoorsClosed();
    CORO_YIELD(0.25);

    for(static_i = 0; static_i < puzzle->GetNumTokens(); static_i++)
    {
        float dt;
        while((dt = Game::GetCube(static_i)->OpenShutters(&Background)) >= 0)
        {
            CORO_YIELD(dt);
        }

        new(tokenViewBuffer[static_i]) TokenView(Game::GetCube(static_i), puzzle->GetToken(static_i), true);
        CORO_YIELD(0.1f);

    }

    { // flourish in
        CORO_YIELD(1.1f);
        // show total
        AudioPlayer::PlaySfx(sfx_Tutorial_Mix_Nums);

        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {
            puzzle->GetToken(i)->GetTokenView()->ShowOverlay();
        }
        CORO_YIELD(3.0f);
        for(int i = 0; i < puzzle->GetNumTokens(); i++)
        {
            puzzle->GetToken(i)->GetTokenView()->HideOverlay();
        }
    }

    { // gameplay
        // subscribe to events
        Game::GetInstance().neighborEventHandler = &neighborEventHandler;
        { // game loop
            //TODO                    var pauseHelper = new PauseHelper();
            while(!puzzle->IsComplete()) {
                // most stuff is handled by events and view updaters
                CORO_YIELD(0);
#if 0 //TODO
                // should pause?
                pauseHelper.Update();
                if (pauseHelper.IsTriggered) {
                    mPaused = true;

                    // transition out
                    for(var i=0; i<puzzle.tokens.Length; ++i) {
                        foreach(var dt in game.CubeSet[i].CloseShutters()) { yield return dt; }
                        new BlankView(game.CubeSet[i]);
                    }

                    // do menu
                    using (var menu = new ConfirmationMenu("Return to Menu?", game)) {
                    while(!menu.IsDone) {
                    yield return 0;
                    menu.Tick(game.dt);
                }
                if (menu.Result) {
                    game.sceneMgr.QueueTransition("Quit");
                    yield break;
                }
            }

            // transition back
            for(int i=0; i<puzzle.tokens.Length; ++i) {
                foreach (var dt in game.CubeSet[i].OpenShutters("background")) { yield return dt; }
                puzzle.tokens[i].GetView().Cube = game.CubeSet[i];
                yield return 0.1f;
            }

            pauseHelper.Reset();
            mPaused = false;
        }
#endif
    }

}
// unsubscribe
Game::GetInstance().neighborEventHandler = NULL;
}


{ // flourish out
CORO_YIELD(1);
AudioPlayer::PlaySfx(sfx_Level_Clear);
for(int i = 0; i < puzzle->GetNumTokens(); i++)
{
    puzzle->GetToken(i)->GetTokenView()->ShowLit();
}
CORO_YIELD(3);
}

mTransitioningOut = true;

{ // transition out
for(static_i = 0; static_i < puzzle->GetNumTokens(); static_i++)
{
    float dt;
    while((dt = Game::GetCube(static_i)->CloseShutters(&Background)) >= 0)
    {
        CORO_YIELD(dt);
    }
    CORO_YIELD(0.1f);
}
}

if (!game->IsPlayingRandom())
{ // closing remarks
    int count = puzzle->CountAfterThisInChapterWithCurrentCubeSet();
    if (count > 0)
    {
#if 0 //todo
        var nv = new NarratorView();
        nv.Cube = game.CubeSet[0];
        const float kTransitionTime = 0.2f;
        Jukebox.PlayShutterOpen();
        for(var t=0f; t<kTransitionTime; t+=game.dt) {
            nv.SetTransitionAmount(t/kTransitionTime);
            yield return 0;
        }
        nv.SetTransitionAmount(1f);
        yield return 0.5f;
        if (count == 1) {
            nv.SetMessage("1 code to go...", "mix01");
            int i=0;
            yield return 0;
            for(var t=0f; t<3f; t+=game.dt) {
                i = 1-i;
                nv.SetEmote("mix0"+(i+1));
                yield return 0;
            }
        } else {
            nv.SetMessage(string.Format("{0} codes to go...", count));
            yield return 2f;
        }
        Jukebox.PlayShutterClose();
        for(var t=0f; t<kTransitionTime; t+=game.dt) {
            nv.SetTransitionAmount(1f-t/kTransitionTime);
            yield return 0;
        }
        nv.SetTransitionAmount(0f);
#endif
    }
}

Transition("Complete");


CORO_END
return -1;
}

//-------------------------------------------------------------------------
// EVENTS
//-------------------------------------------------------------------------
void PuzzleController::NeighborEventHandler::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    if (owner->IsPaused() /*TODO|| mRemoveEventBuffer.Count > 0*/) { return; }

    // validate args
    TokenView *v = (TokenView*)Game::GetCube(c0)->GetView();
    TokenView *nv = (TokenView*)Game::GetCube(c1)->GetView();
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
    /*TODO          if (owner->IsPaused() TODO|| mRemoveEventBuffer.Count > 0)
            {
                mRemoveEventBuffer.Add(new RemoveEvent() { c = c, s = s, nc = nc, ns = ns });
                return;
            }
*/

    // validate args
    TokenView *v = (TokenView*)Game::GetCube(c0)->GetView();
    TokenView *nv = (TokenView*)Game::GetCube(c1)->GetView();
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
        for(int i = 0; i < owner->puzzle->GetNumTokens(); i++)
        {
            Token *token = owner->puzzle->GetToken(i);
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
/*TODO
        void ProcessRemoveEventBuffer() {
            var args = mRemoveEventBuffer;
            mRemoveEventBuffer = new List<RemoveEvent>(args.Count);
            foreach(var arg in args) {
                OnNeighborRemove(arg.c, arg.s, arg.nc, arg.ns);
            }
        } */

//-------------------------------------------------------------------------
// CLEANUP
//-------------------------------------------------------------------------

void PuzzleController::OnTick (float dt)
{
    /* TODO			if (!IsPaused && mRemoveEventBuffer.Count > 0) {
            ProcessRemoveEventBuffer();
            } */
    UPDATE_CORO(TheBigCoroutine, dt);
    Game::UpdateCubeViews(dt);
}

void PuzzleController::OnPaint (bool canvasDirty)
{
    //if (canvasDirty) { game.CubeSet.PaintViews(); }
    if(!mTransitioningOut)
    {   //vault door drawing inside update during transitition out
        Game::PaintCubeViews();
    }
}

void PuzzleController::Transition(const char *id)
{
    game->sceneMgr.QueueTransition(id);
}

void PuzzleController::OnDispose ()
{
    //mRemoveEventBuffer.Clear();
    //puzzle.ClearUserdata();
    //puzzle = null;
    //mPaused = false;
    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();
    delete puzzle;
}

}


