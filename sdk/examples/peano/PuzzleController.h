#pragma once

#include "StateMachine.h"
#include "PuzzleHelper.h"
#include "AudioPlayer.h"
#include "TokenView.h"
#include "Game.h"

namespace TotalsGame {

class PauseHelper;

namespace PuzzleController
{       

    Game::GameState Coroutine();

}
}

