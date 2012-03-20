#pragma once

#include "sifteo.h"
#include "assets.gen.h"
#include "Skins.h"

namespace TotalsGame
{
namespace Skins
{
struct Skin
{
    const AssetImage &vault_door;
    const AssetImage &background;
    const AssetImage &background_lit;
    const AssetImage &accent;

    const PinnedAssetImage &lit_bottom;
    const PinnedAssetImage &lit_bottom_add;
    const PinnedAssetImage &lit_bottom_sub;
    const PinnedAssetImage &lit_bottom_mul;
    const PinnedAssetImage &lit_bottom_div;
    const PinnedAssetImage &lit_left;
    const PinnedAssetImage &lit_right;
    const PinnedAssetImage &lit_right_add;
    const PinnedAssetImage &lit_right_sub;
    const PinnedAssetImage &lit_right_mul;
    const PinnedAssetImage &lit_right_div;
    const PinnedAssetImage &lit_top;

    const PinnedAssetImage &unlit_bottom;
    const PinnedAssetImage &unlit_bottom_add;
    const PinnedAssetImage &unlit_bottom_sub;
    const PinnedAssetImage &unlit_bottom_mul;
    const PinnedAssetImage &unlit_bottom_div;
    const PinnedAssetImage &unlit_left;
    const PinnedAssetImage &unlit_right;
    const PinnedAssetImage &unlit_right_add;
    const PinnedAssetImage &unlit_right_sub;
    const PinnedAssetImage &unlit_right_mul;
    const PinnedAssetImage &unlit_right_div;
    const PinnedAssetImage &unlit_top;
};

enum SkinType
{
    SkinType_Default,
    SkinType_Blue,
    SkinType_Green,
    SkinType_Purple,
    SkinType_Red,
    SkinType_Turquoise,
    NumSkins
};

void SetSkin(SkinType skinType);
const Skin& GetSkin();

}
}






