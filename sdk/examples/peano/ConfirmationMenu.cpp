#include "ConfirmationMenu.h"
#include "Skins.h"
#include "DialogWindow.h"

namespace TotalsGame {

namespace ConfirmationMenu
{

const int YES = 1;
const int NO = 2;
const int ASK = 0;

bool gotTouchOn[3];
bool triggered[3];

static const float kTransitionTime = 0.333f;

const int kPad = 1;

void OnCubeTouch(void *, Cube::ID cid)
{
    bool pressed = Game::cubes[cid].touching();
    if(cid == YES || cid ==NO)
    {
        if(pressed)
            gotTouchOn[cid] = true;

        if (!pressed && gotTouchOn[cid])
        {
            triggered[cid] = true;
            PLAY_SFX(sfx_Menu_Tilt_Stop);
        }
    }
}

int CollapsesPauses(int off) {
  if (off > 7) {
    if (off < 7 + kPad) {
      return 7;
    }
    off -= kPad;
  }
  return off;
}

void PaintTheDoors(TotalsCube *c, int offset, bool opening)
{
    offset = CollapsesPauses(offset);

    const Skins::Skin &skin = Skins::GetSkin();
    if (offset < 7) {
        // moving to the left
        c->ClipImage(&skin.vault_door, Vec2(7-offset, 7));
        c->ClipImage(&skin.vault_door, Vec2(7-16-offset, 7));
        c->ClipImage(&skin.vault_door, Vec2(7-offset, 7-16));
        c->ClipImage(&skin.vault_door, Vec2(7-16-offset,7-16));
    } else {
        // opening
        int off = offset - 7;
        int top = 7 - off;
        int bottom = 7 + MIN(5, off);

        if (opening && bottom-top > 0)
        {
            c->FillArea(&Dark_Purple, Vec2(0, top), Vec2(16, bottom-top));
        }

        c->ClipImage(&skin.vault_door, Vec2(0, top-16));
        c->ClipImage(&skin.vault_door, Vec2(0, bottom));
    }
}

void AnimateDoors(TotalsCube *c, bool opening)
{
    if(opening)
        AudioPlayer::PlayShutterOpen();
    else
        AudioPlayer::PlayShutterClose();
    for(float t=0; t<kTransitionTime; t+=Game::dt) {
        float amount = opening ? t/kTransitionTime : 1-t/kTransitionTime;

        PaintTheDoors(c, (7+6+kPad) * amount, opening);
        System::paintSync();
        Game::UpdateDt();
    }
    PaintTheDoors(c, opening? (7+6+kPad): 0, opening);
    System::paintSync();
}

//true means selected first choice (yes)
bool Run(const char *msg, const AssetImage *choice1, const AssetImage *choice2)
{    
    bool result = false;

    if(!choice1)
    {
        choice1 = &Icon_Yes;
    }

    if(!choice2)
    {
        choice2 = &Icon_No;
    }

    AnimateDoors(Game::cubes+YES, true);
    Game::cubes[YES].Image(*choice1, Vec2(3, 1));

    AnimateDoors(Game::cubes+NO, true);
    Game::cubes[NO].Image(*choice2, Vec2(3, 1));
    System::paint();

    DialogWindow dw(Game::cubes+ASK);

    if(msg)
    {   //no message -> dont mess with cube 0 (for tutorial)
        AnimateDoors(Game::cubes+ASK, true);        
        dw.SetBackgroundColor(75, 0, 85);
        dw.SetForegroundColor(255, 255, 255);
        dw.DoDialog(msg, 16, 20);
    }

    gotTouchOn[0] = gotTouchOn[1] = gotTouchOn[2] = false;
    triggered[0] = triggered[1] = triggered[2] = false;
    void *oldTouch = _SYS_getVectorHandler(_SYS_CUBE_TOUCH);
    _SYS_setVector(_SYS_CUBE_TOUCH, (void*)&OnCubeTouch, NULL);

    while(!(triggered[YES] || triggered[NO])) {
        System::yield();
        Game::UpdateDt();
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, oldTouch, NULL);

    AnimateDoors(Game::cubes+YES, false);
    AnimateDoors(Game::cubes+NO, false);

    if(msg)
    {
        dw.EndIt();
        Game::cubes[ASK].FillArea(&Dark_Purple, Vec2(0, 0), Vec2(16, 16));
        AnimateDoors(Game::cubes+ASK, false);
    }

    return triggered[YES];

}



}

}

