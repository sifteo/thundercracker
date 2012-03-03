#pragma once

#include "InterstitialView.h"
#include "assets.gen.h"
#include"Game.h"
#include "AudioPlayer.h"
#include "coroutine.h"
#include "MenuController.h"

namespace TotalsGame
{

namespace ConfirmationMenu
{

bool Coroutine(const char *msg);


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

    ConfirmationChoiceView(TotalsCube *c, const AssetImage *_image);

    void DidAttachToCube (TotalsCube *c);
    void WillDetachFromCube (TotalsCube *c);

    void OnButton(TotalsCube *c, bool pressed);

    void Paint ();

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}

};

}
}

