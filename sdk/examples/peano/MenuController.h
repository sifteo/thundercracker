#pragma once

#include "Game.h"

namespace TotalsGame {

namespace MenuController
{


Game::GameState Run();


class TransitionView : public View
{
    static const int kPad = 1;

public:

    TransitionView(TotalsCube *c);
    virtual ~TransitionView() {}

    void SetTransitionAmount(float u);

    int mOffset;
    bool mBackwards;


    bool GetIsLastFrame();
private:
    void SetTransition(int offset);

    int CollapsesPauses(int off);
public:
    void Paint();

    //for placement new
    void* operator new (size_t size, void* ptr) throw() {return ptr;}
    void operator delete(void *ptr) {}
};
}


}

