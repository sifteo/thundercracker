#pragma once

#include "StateMachine.h"
#include "PuzzleHelper.h"
#include "AudioPlayer.h"
#include "TokenView.h"

namespace TotalsGame {

class PauseHelper;
class ConfirmationMenu;

class PuzzleController: public IStateController
{       
private:
    CORO_PARAMS;
    int static_i;

    PauseHelper *pauseHelper;
    ConfirmationMenu *menu;

    class EventHandler: public TotalsCube::EventHandler
    {
        void OnCubeShake(TotalsCube *cube) {};
        void OnCubeTouch(TotalsCube *cube, bool touching) {};
        //TODO connect, disconnect
    };
    EventHandler eventHandlers[Game::NUMBER_OF_CUBES];

    class NeighborEventHandler: public Game::NeighborEventHandler
    {
        PuzzleController *owner;
        void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
        void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
    public:
        NeighborEventHandler(PuzzleController *p) {owner = p;}
    };
    NeighborEventHandler neighborEventHandler;

public:
    //-------------------------------------------------------------------------
    // PARAMETERS
    //-------------------------------------------------------------------------

    Game *game;
    Puzzle *puzzle;
    bool mTransitioningOut;
    bool mPaused;

    bool IsPaused();
    //-------------------------------------------------------------------------
    // SETUP
    //-------------------------------------------------------------------------

    PuzzleController(Game *_game);

    virtual void OnSetup ();

    float TheBigCoroutine();

    //-------------------------------------------------------------------------
    // CLEANUP
    //-------------------------------------------------------------------------

    virtual void OnTick ();

    virtual void OnPaint (bool canvasDirty);

    void Transition(const char *id);

    virtual void OnDispose ();
};
}

