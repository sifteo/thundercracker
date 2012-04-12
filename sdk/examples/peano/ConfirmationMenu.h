#pragma once

#include "InterstitialView.h"
#include "assets.gen.h"
#include "Game.h"
#include "AudioPlayer.h"
#include "MenuController.h"

namespace TotalsGame
{

namespace ConfirmationMenu
{

bool Run(const char *msg, const Sifteo::AssetImage *choice1=NULL, const Sifteo::AssetImage *choice2=NULL);

}
}

