#include "ConfirmationMenu.h"
#include "Skins.h"
#include "DialogWindow.h"

namespace TotalsGame {

namespace ConfirmationMenu
{

const int YES = 0;
const int NO = 1;
const int ASK = 2;

bool gotTouchOn[2];
bool triggered[2];

static const float kTransitionTime = 0.333f;

void OnCubeTouch(Cube::ID cube, bool pressed)
{
    if(pressed)
        gotTouchOn[cube] = true;

    if (!pressed && gotTouchOn[cube])
    {
        triggered[cube] = true;
        PLAY_SFX(sfx_Menu_Tilt_Stop);
    }
}


void PaintTheDoors(TotalsCube *c, int offset, bool opening)
{
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
    const int kPad = 1;
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


bool Run(const char *msg)
{    
    bool result = false;

    AnimateDoors(Game::cubes+YES, true);
    Game::cubes[YES].Image(Icon_Yes, Vec2(3, 1));

    AnimateDoors(Game::cubes+NO, true);
    Game::cubes[NO].Image(Icon_No, Vec2(3, 1));

    AnimateDoors(Game::cubes+ASK, true);
    DialogWindow dw(Game::cubes+ASK);
    dw.SetBackgroundColor(75, 0, 85);
    dw.SetForegroundColor(255, 255, 255);
    dw.DoDialog(msg, 16, 20);

    gotTouchOn[0] = gotTouchOn[1] = false;
    triggered[0] = triggered[1] = false;
    _SYS_setVector(_SYS_CUBE_TOUCH, (void*)&OnCubeTouch, NULL);

    while(!(triggered[0] || triggered[1])) {
        System::yield();
        Game::UpdateDt();
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, NULL, NULL);

    AnimateDoors(Game::cubes+YES, true);
    AnimateDoors(Game::cubes+NO, true);

    dw.EndIt();

    AnimateDoors(Game::cubes+ASK, false);

    return triggered[YES];

}



}

}

