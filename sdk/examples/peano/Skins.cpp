#include "assets.gen.h"
#include "Skins.h"
#include "Game.h"

namespace TotalsGame
{
namespace Skins
{

#define SKIN(name) \
{\
    Skin_##name##_VaultDoor,\
    Skin_##name##_Background,\
    Skin_##name##_Background_Lit,\
    Skin_##name##_Accent,\
    Skin_##name##_Lit_Bottom,\
    Skin_##name##_Lit_Bottom_Add,\
    Skin_##name##_Lit_Bottom_Sub,\
    Skin_##name##_Lit_Bottom_Mul,\
    Skin_##name##_Lit_Bottom_Div,\
    Skin_##name##_Lit_Left,\
    Skin_##name##_Lit_Right,\
    Skin_##name##_Lit_Right_Add,\
    Skin_##name##_Lit_Right_Sub,\
    Skin_##name##_Lit_Right_Mul,\
    Skin_##name##_Lit_Right_Div,\
    Skin_##name##_Lit_Top,\
    Skin_##name##_Unlit_Bottom,\
    Skin_##name##_Unlit_Bottom_Add,\
    Skin_##name##_Unlit_Bottom_Sub,\
    Skin_##name##_Unlit_Bottom_Mul,\
    Skin_##name##_Unlit_Bottom_Div,\
    Skin_##name##_Unlit_Left,\
    Skin_##name##_Unlit_Right,\
    Skin_##name##_Unlit_Right_Add,\
    Skin_##name##_Unlit_Right_Sub,\
    Skin_##name##_Unlit_Right_Mul,\
    Skin_##name##_Unlit_Right_Div,\
    Skin_##name##_Unlit_Top\
}

const Skin skins[NumSkins] =
{
    SKIN(Default),
    SKIN(blue)
};


SkinType currentSkin = SkinType_Default;

void SetSkin(SkinType skinType)
{
    if(skinType == currentSkin)
        return;

    currentSkin = skinType;
    const Skin& s = GetSkin();

    //animate transition to new skin
    for(int c = 0; c < NUM_CUBES; c++)
    {
        TotalsCube *cube = Game::cubes+c;
        //bring in from top and bottom
        for(int frame = 0; frame <= 9; frame++)
        {
            cube->ClipImage(&s.vault_door, Vec2(0, -18 + frame));
            cube->ClipImage(&s.vault_door, Vec2(0, 16 - frame));
            Game::Wait(0);
        }

        //slide it over a bit
        for(int frame = 0; frame <= 9; frame++)
        {
            cube->ClipImage(&s.vault_door, Vec2(-frame, -9));
            cube->ClipImage(&s.vault_door, Vec2(-frame, 7));

            cube->ClipImage(&s.vault_door, Vec2(16-frame, -9));
            cube->ClipImage(&s.vault_door, Vec2(16-frame, 7));

            Game::Wait(0);
        }

    }
}

const Skin& GetSkin()
{
    return skins[currentSkin];
}

}
}







