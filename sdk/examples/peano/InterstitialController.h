#pragma once

#include "StateMachine.h"
#include "Game.h"
#include "InterstitialView.h"
#include "BlankView.h"
#include "coroutine.h"
#include "AudioPlayer.h"

namespace TotalsGame {

class InterstitialController : public IStateController {
    bool mDone;
    InterstitialView *iv;

    CORO_PARAMS;
    float remembered_t;

public:
    InterstitialController ();

    void OnSetup();

    static Game::GameState Coroutine();

    void OnTick ();
    void OnPaint (bool canvasDirty);
    void OnDispose ();

};


}

