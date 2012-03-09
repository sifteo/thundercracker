#include "ConfirmationMenu.h"

namespace TotalsGame {

namespace ConfirmationMenu
{

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
    if (!pressed)
    {
        mTriggered = true;
        PLAY_SFX(sfx_Menu_Tilt_Stop);
    }
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

    static const float kTransitionTime = 0.333f;
    bool result;


    ConfirmationChoiceView yes(&Icon_Yes), no(&Icon_No);
    Game::cubes[0].SetView(&yes);
    Game::cubes[1].SetView(&no);

    result = false;

    InterstitialView label;
    Game::cubes[2].SetView(&label);
    label.message = msg;


    AudioPlayer::PlayShutterOpen();
    label.TransitionSync(kTransitionTime, true);

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        yes.SetTransitionAmount(t/kTransitionTime);
        yes.Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    yes.SetTransitionAmount(1);
    yes.Paint();

    AudioPlayer::PlayShutterOpen();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        no.SetTransitionAmount(t/kTransitionTime);
        no.Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    no.SetTransitionAmount(1);
    no.Paint();   //need to render one last frame before we go synchronous

    while(!(yes.Triggered() || no.Triggered())) {
        System::paint();
        Game::UpdateDt();
    }
    result = yes.Triggered();

    first = result ? &no : &yes;
    second = result ? &yes : &no;

    AudioPlayer::PlayShutterClose();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        first->SetTransitionAmount(1-t/kTransitionTime);
        first->Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    first->SetTransitionAmount(0);
    first->Paint();

    AudioPlayer::PlayShutterClose();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        second->SetTransitionAmount(1-t/kTransitionTime);
        second->Paint();
        System::paintSync();
        Game::UpdateDt();
    }
    second->SetTransitionAmount(0);
    second->Paint();

    AudioPlayer::PlayShutterClose();
    label.TransitionSync(kTransitionTime, false);

    return result;

}



}

}

