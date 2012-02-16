#pragma once

#include "StateMachine.h"
#include "View.h"
#include "coroutine.h"
#include "TiltFlowDetailView.h"
#include "TiltFlowMenu.h"
#include "BlankView.h"
#include "ObjectPool.h"

namespace TotalsGame {

class TotalsCube;

  class MenuController : public IStateController
  {

    class TextFieldView : View
    {
    public:
      void Paint(TotalsCube *c);
    };


    class TransitionView : public View
    {
        static const int kPad = 1;

        DECLARE_POOL(TransitionView, 2);

    public:

        TransitionView(TotalsCube *c);

      void SetTransitionAmount(float u);

      int mOffset;
      bool mBackwards;


      bool GetIsLastFrame();
  private:
      void SetTransition(int offset);

      int CollapsesPauses(int off);
  public:
      void Paint (TotalsCube *c);
    };

private:

    CORO_PARAMS;
    float rememberedT;

     TransitionView *tv;
     TiltFlowDetailView *labelView;
     TiltFlowMenu *menu;

    Game *mGame;
  public:
    Game *GetGame();

    MenuController(Game *game);

    void OnSetup();

/* todo
    void OnCubeLost(Cube cube) {
      if (!cube.IsUnused()) {
        cube.MoveViewTo(mGame.CubeSet.FindAnyIdle());
      }
    } */

    float Coroutine(float dt);


    float OnTick(float dt);

    void OnPaint(bool canvasDirty);

    void OnDispose();
  };


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

}

