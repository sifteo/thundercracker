#pragma once

#include "InterstitialView.h"
#include "assets.gen.h"
#include"Game.h"
#include "AudioPlayer.h"
#include "coroutine.h"
#include "MenuController.h"

namespace TotalsGame {



class ConfirmationChoiceView : public MenuController::TransitionView {
    const AssetImage *image;
    bool mTriggered;
public:
    bool Triggered() { return mTriggered; }

    ConfirmationChoiceView(TotalsCube *c, const AssetImage *_image):
        MenuController::TransitionView(c)
    {
        image = _image;
        mTriggered = false;
    }

    void DidAttachToCube (TotalsCube *c) {
        //TODO c.ButtonEvent += OnButton;
    }

    void WillDetachFromCube (TotalsCube *c) {
        //TODO c.ButtonEvent -= OnButton;
    }

    void OnButton(TotalsCube *c, bool pressed) {
        if (!pressed) {
            mTriggered = true;
            AudioPlayer::PlaySfx(sfx_Menu_Tilt_Stop);
        }
    }

    void Paint () {
        TransitionView::Paint();
        if (GetIsLastFrame()) {
            GetCube()->Image(image, Vec2(24, 12));
        }
    }

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}

};


class ConfirmationMenu {
    InterstitialView *mLabel;
    ConfirmationChoiceView *mYes;
    ConfirmationChoiceView *mNo;
    static const float kTransitionTime = 0.333f;
    bool mResult;
    bool mManagingLabel;
    bool done;

    CORO_PARAMS;
    float remembered_t;
    ConfirmationChoiceView *first;
    ConfirmationChoiceView *second;
public:

    ConfirmationMenu (const char *msg) {
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

    void Tick(float dt) {
        Coroutine(dt);
        if(!done)
        {
             mYes->Update(dt);
            mNo->Update(dt);
            mLabel->Update(dt);
        }
    }

    float Coroutine(float dt) {

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

    bool IsDone() {
        return done;
    }

    bool GetResult() {
        return mResult;
    }

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}

};



}

