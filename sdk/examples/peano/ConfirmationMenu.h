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

bool Run(const char *msg);


class ConfirmationChoiceView : public MenuController::TransitionView {
    const AssetImage *image;
    bool mTriggered;

    class EventHandler: public TotalsCube::EventHandler
    {
        ConfirmationChoiceView *owner;
    public:
        EventHandler(ConfirmationChoiceView *_owner);
        void OnCubeTouch(TotalsCube *c, bool pressed);
    };
    EventHandler eventHandler;

public:
    bool Triggered();

    ConfirmationChoiceView(const AssetImage *_image);
    virtual ~ConfirmationChoiceView() {}

    void DidAttachToCube (TotalsCube *c);
    void WillDetachFromCube (TotalsCube *c);

    void OnButton(TotalsCube *c, bool pressed);

    void Paint ();

};

}
}

