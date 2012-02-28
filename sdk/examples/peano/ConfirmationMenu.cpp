#include "ConfirmationMenu.h"

namespace TotalsGame {


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

ConfirmationChoiceView::ConfirmationChoiceView(TotalsCube *c, const PinnedAssetImage *_image):
    MenuController::TransitionView(c), eventHandler(this)
{
    image = _image;
    mTriggered = false;

    DidAttachToCube(c);
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
        AudioPlayer::PlaySfx(sfx_Menu_Tilt_Stop);
    }
}

void ConfirmationChoiceView::Paint ()
{
    TransitionView::Paint();
    if (GetIsLastFrame())
    {
        GetCube()->Image(image, Vec2(3, 1)); //TODO 1 is a bad approximationg for 12/8
    }
}



ConfirmationMenu::ConfirmationMenu (const char *msg)
{
    static char buffers[2][sizeof(ConfirmationChoiceView)];
    mYes = new(buffers[0]) ConfirmationChoiceView(Game::GetCube(0), &Icon_Yes);
    mNo = new(buffers[1]) ConfirmationChoiceView(Game::GetCube(1), &Icon_No);
    mResult = false;
    mManagingLabel = false;

    static char ivBuffer[sizeof(InterstitialView)];
    mLabel = new(ivBuffer) InterstitialView(Game::GetCube(2));
    mManagingLabel= true;

    mLabel->image = NULL;
    mLabel->message = msg;

    done = false;

    CORO_RESET;
}

void ConfirmationMenu::Tick(float dt)
{
    Coroutine(dt);
    if(!done)
    {
        mYes->Update(dt);
        mNo->Update(dt);
        mLabel->Update(dt);
    }
}

float ConfirmationMenu::Coroutine(float dt)
{

    CORO_BEGIN;

    AudioPlayer::PlayShutterOpen();
    for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=dt) {
        mLabel->SetTransitionAmount(remembered_t/kTransitionTime);
        CORO_YIELD(0);
    }
    mLabel->SetTransitionAmount(1);
    CORO_YIELD(0);
    AudioPlayer::PlayShutterOpen();
    for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=dt) {
        mYes->SetTransitionAmount(remembered_t/kTransitionTime);
        CORO_YIELD(0);
    }
    mYes->SetTransitionAmount(1);
    CORO_YIELD(0);
    AudioPlayer::PlayShutterOpen();
    for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=dt) {
        mNo->SetTransitionAmount(remembered_t/kTransitionTime);
        CORO_YIELD(0);
    }
    mNo->SetTransitionAmount(1);
    CORO_YIELD(0);
    while(!(mYes->Triggered() || mNo->Triggered())) {
        CORO_YIELD(0);
    }
    mResult = mYes->Triggered();

    first = mResult ? mNo : mYes;
    second = mResult ? mYes : mNo;

    AudioPlayer::PlayShutterClose();
    for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=dt) {
        first->SetTransitionAmount(1-remembered_t/kTransitionTime);
        CORO_YIELD(0);
    }
    first->SetTransitionAmount(0);
    CORO_YIELD(0);

    AudioPlayer::PlayShutterClose();
    for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=dt) {
        second->SetTransitionAmount(1-remembered_t/kTransitionTime);
        CORO_YIELD(0);
    }
    second->SetTransitionAmount(0);
    CORO_YIELD(0);

    AudioPlayer::PlayShutterClose();
    for(remembered_t=0; remembered_t<kTransitionTime; remembered_t+=dt) {
        mLabel->SetTransitionAmount(1-remembered_t/kTransitionTime);
        CORO_YIELD(0);
    }
    mLabel->SetTransitionAmount(0);

    CORO_END;

    done = true;
    return -1;
}

bool ConfirmationMenu::IsDone()
{
    return done;
}

bool ConfirmationMenu::GetResult()
{
    return mResult;
}




}

