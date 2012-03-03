#include "PuzzleController.h"
#include "BlankView.h"
#include "assets.gen.h"
#include "PauseHelper.h"
#include "ConfirmationMenu.h"

namespace TotalsGame {
    
    namespace PuzzleController
    {
        
        bool IsPaused()
        {
            return mPaused;
        }
        
        
        void OnSetup ()
        {
            CORO_RESET;
            
            mPaused = false;
            pauseHelper = NULL;
            
            AudioPlayer::PlayInGameMusic();
            mTransitioningOut = false;
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
            mPaused = false;
            puzzle->hintsUsed = 0;
            
            for(int i = 0; i < NUM_CUBES; i++)
            {
                Game::cubes[0].AddEventHandler(&eventHandlers[i]);
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
        
        Game::GameState Coroutine()
        {
            static char blankViewBuffer[NUM_CUBES][sizeof(BlankView)];
            static char tokenViewBuffer[NUM_CUBES][sizeof(TokenView)];
            static char pauseHelperBuffer[sizeof(PauseHelper)];
            
            CORO_BEGIN;
            
            for(int i = puzzle->GetNumTokens(); i < NUM_CUBES; i++)
            {
                new(blankViewBuffer[i]) BlankView(&Game::cubes[0], NULL);
            }
            Game::DrawVaultDoorsClosed();
            CORO_YIELD(0.25);
            
            for(static_i = 0; static_i < puzzle->GetNumTokens(); static_i++)
            {
                Game::cubes[static_i].OpenShuttersSync(&Background);
                {
                    TokenView *tv = new(tokenViewBuffer[static_i]) TokenView(&Game::cubes[static_i], puzzle->GetToken(static_i), true);
                    tv->PaintNow();
                }
                
                CORO_YIELD(0.1f);
                
            }
            
            { // flourish in
                CORO_YIELD(1.1f);
                // show total
                PLAY_SFX(sfx_Tutorial_Mix_Nums);
                
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
                pauseHelper = new(pauseHelperBuffer) PauseHelper();
                
                // subscribe to events
                Game::neighborEventHandler = &neighborEventHandler;
                { // game loop
                    while(!puzzle->IsComplete()) {
                        // most stuff is handled by events and view updaters
                        CORO_YIELD(0);
                        
                        // should pause?
                        pauseHelper->Update();
                        if (pauseHelper->IsTriggered())
                        {
                            mPaused = true;
                            
                            // transition out
                            for(static_i=0; static_i<puzzle->GetNumTokens(); ++static_i)
                            {
                                for(int i = 0; i < _SYS_VRAM_SPRITES; i++)
                                {
                                    Game::cubes[static_i].backgroundLayer.hideSprite(i);
                                }
                                Game::cubes[static_i].foregroundLayer.Clear();
                                Game::cubes[static_i].foregroundLayer.Flush();
                                Game::cubes[static_i].SetView(NULL);
                                
                                Game::cubes[static_i].CloseShuttersSync(&Background);
                                new(blankViewBuffer[static_i]) BlankView(&Game::cubes[static_i], NULL);
                            }
                            
                            // do menu
                            {
                                static char confirmationBuffer[sizeof(ConfirmationMenu)];
                                menu = new(confirmationBuffer) ConfirmationMenu("Return to Menu?");
                                
                                while(!menu->IsDone())
                                {
                                    System::paint();
                                    Game::UpdateDt();
                                    menu->Tick();
                                    
                                    System::paint();
                                    Game::UpdateDt();
                                    menu->Tick();
                                    Game::UpdateCubeViews();
                                    Game::PaintCubeViews();
                                    for(int i = 0; i < NUM_CUBES; i++)
                                    {
                                        Game::cubes[0].foregroundLayer.Flush();
                                    }
                                }
                            }
                            
                            if (menu->GetResult()) {
                                Game::neighborEventHandler = NULL;
                                return Game::GameState_Menu;
                            }
                            
                            
                            // transition back
                            for(static_i=0; static_i<puzzle->GetNumTokens(); ++static_i)
                            {
                                Game::cubes[static_i].SetView(NULL);
                                
                                Game::cubes[static_i].OpenShuttersSync(&Background);
                                
                                Game::cubes[static_i].SetView(puzzle->GetToken(static_i)->GetTokenView());
                                ((TokenView*)Game::cubes[static_i].GetView())->PaintNow();
                                CORO_YIELD(0.1f);
                            }
                            
                            pauseHelper->Reset();
                            mPaused = false;
                        }
                    }
                }
                
            }
            // unsubscribe
            Game::neighborEventHandler = NULL;
            
            
            { // flourish out
                CORO_YIELD(1);
                PLAY_SFX(sfx_Level_Clear);
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
                    puzzle->GetToken(static_i)->GetTokenView()->GetCube()->foregroundLayer.Clear();
                    puzzle->GetToken(static_i)->GetTokenView()->GetCube()->foregroundLayer.Flush();
                    
                    Game::cubes[static_i].CloseShuttersSync(&Background);
                    CORO_YIELD(0.1f);
                }
            }
            
            if (!Game::IsPlayingRandom())
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
            
            
            CORO_END;
            return Game::GameState_Advance;
        }
        
        //-------------------------------------------------------------------------
        // EVENTS
        //-------------------------------------------------------------------------
        void PuzzleController::NeighborEventHandler::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            if (IsPaused() /*TODO|| mRemoveEventBuffer.Count > 0*/) { return; }
            
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
            /*TODO          if (IsPaused() TODO|| mRemoveEventBuffer.Count > 0)
             {
             mRemoveEventBuffer.Add(new RemoveEvent() { c = c, s = s, nc = nc, ns = ns });
             return;
             }
             */
            
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
        
        
        void OnPaint (bool canvasDirty)
        {
            //if (canvasDirty) { game.CubeSet.PaintViews(); }
            if(!mTransitioningOut)
            {   //vault door drawing inside update during transitition out
                Game::PaintCubeViews();
            }
        }
        
        
        void OnDispose ()
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
    
    
}