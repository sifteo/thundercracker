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
class ConfirmationMenu;

  namespace MenuController
  {

     static const int MAX_CHAPTERS=7;

    class TextFieldView : View
    {
    public:
      void Paint(TotalsCube *c);
    };

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

    float rememberedT;

     TransitionView *tv;
     TiltFlowDetailView *labelView;
     TiltFlowMenu *menu;
     ConfirmationMenu *confirm;

    void OnSetup();

/* todo
    void OnCubeLost(Cube cube) {
      if (!cube.IsUnused()) {
        cube.MoveViewTo(mGame.CubeSet.FindAnyIdle());
      }
    } */

      Game::GameState Coroutine();

    void OnPaint(bool canvasDirty);

    void OnDispose();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

}

