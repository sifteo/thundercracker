#pragma once

#include "StateMachine.h"
#include "PuzzleHelper.h"
#include "AudioPlayer.h"
#include "TokenView.h"
#include "Game.h"

namespace TotalsGame {

class PauseHelper;
class ConfirmationMenu;

namespace PuzzleController
{       
    int static_i;

    PauseHelper *pauseHelper;
    ConfirmationMenu *menu;

    class EventHandler: public TotalsCube::EventHandler
    {
        void OnCubeShake(TotalsCube *cube) {};
        void OnCubeTouch(TotalsCube *cube, bool touching) {};
        //TODO connect, disconnect
    };
    EventHandler eventHandlers[NUM_CUBES];

    class NeighborEventHandler: public Game::NeighborEventHandler
    {
        void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
        void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
    };
    NeighborEventHandler neighborEventHandler;

    //-------------------------------------------------------------------------
    // PARAMETERS
    //-------------------------------------------------------------------------

    Puzzle *puzzle;
    bool mTransitioningOut;
    bool mPaused;

    bool IsPaused();
    //-------------------------------------------------------------------------
    // SETUP
    //-------------------------------------------------------------------------

    void OnSetup ();

    Game::GameState Coroutine();

    //-------------------------------------------------------------------------
    // CLEANUP
    //-------------------------------------------------------------------------

    void OnTick ();

    void OnPaint (bool canvasDirty);

    void Transition(const char *id);

    void OnDispose ();
}
}

