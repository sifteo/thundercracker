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

    TransitionView();
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

};
}


}

