#include "assets.gen.h"
#include "Skins.h"

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
    currentSkin = skinType;
}

const Skin& GetSkin()
{
    return skins[currentSkin];
}

}
}







