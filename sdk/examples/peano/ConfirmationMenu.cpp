#include "ConfirmationMenu.h"

namespace TotalsGame {

namespace ConfirmationMenu
{

static const float kTransitionTime = 0.333f;

ConfirmationChoiceView *first;
ConfirmationChoiceView *second;


ConfirmationChoiceView::EventHandler::EventHandler(ConfirmationChoiceView *_owner)
{
    owner = _owner;
}

void ConfirmationChoiceView::EventHandler::OnCubeTouch(TotalsCube *c, bool pressed)
{
    owner->OnButton(c, pressed);
}


bool ConfirmationChoiceView::Triggered()
{
    return mTriggered;
}

ConfirmationChoiceView::ConfirmationChoiceView(const AssetImage *_image):
    eventHandler(this)
{
    image = _image;
    mTriggered = false;
    mGotTouchOn = false;
}

void ConfirmationChoiceView::DidAttachToCube (TotalsCube *c)
{
    c->AddEventHandler(&eventHandler);
}

void ConfirmationChoiceView::WillDetachFromCube (TotalsCube *c)
{
    c->RemoveEventHandler(&eventHandler);
}

void ConfirmationChoiceView::OnButton(TotalsCube *c, bool pressed)
{
    if(pressed)
        mGotTouchOn = true;

    if (!pressed && mGotTouchOn)
    {
        mTriggered = true;
        PLAY_SFX(sfx_Menu_Tilt_Stop);
    }
}

void ConfirmationChoiceView::AnimateDoors(bool opening)
{
    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        SetTransitionAmount(opening ? t/kTransitionTime : 1-t/kTransitionTime);
        Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    SetTransitionAmount(opening? 1 : 0);
    Paint();
}

void ConfirmationChoiceView::Paint ()
{
    TransitionView::Paint();
    if (GetIsLastFrame())
    {
        GetCube()->Image(image, Vec2(3, 1));
    }
}



bool Run(const char *msg)
{    
    bool result;

    ConfirmationChoiceView yes(&Icon_Yes), no(&Icon_No);
    Game::cubes[0].SetView(&yes);
    Game::cubes[1].SetView(&no);

    result = false;

    InterstitialView label;
    Game::cubes[2].SetView(&label);
    label.message = msg;

    label.TransitionSync(kTransitionTime, true);

    yes.AnimateDoors(true);
    no.AnimateDoors(true);

    while(!(yes.Triggered() || no.Triggered())) {
        System::paint();
        Game::UpdateDt();
    }
    result = yes.Triggered();

    first = result ? &no : &yes;
    second = result ? &yes : &no;

    yes.AnimateDoors(false);
    no.AnimateDoors(false);

    label.TransitionSync(kTransitionTime, false);

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return result;

}



}

}

