#include "ui_assets.h"
#include "ui_lowbatt.h"
#include "svmclock.h"
#include "svmloader.h"
#include "batterylevel.h"

const UIMenu::Item UILowBatt::baseItems[NUM_ITEMS] = {
    // Note: labels with an even number of characters will center perfectly
    { "Base battery low", UI_IMAGE_INDEX(iconLowBattBase), true },
    { "Quit",             UI_IMAGE_INDEX(iconQuit),        true },
};

const UIMenu::Item UILowBatt::cubeItems[NUM_ITEMS] = {
    // Note: labels with an even number of characters will center perfectly
    { "Cube battery low", UI_IMAGE_INDEX(iconLowBattCube), true },
    { "Quit",             UI_IMAGE_INDEX(iconQuit),        true },
};

UILowBatt::UILowBatt(UICoordinator &uic) :
    baseMenu(uic, baseItems, NUM_ITEMS),
    cubeMenu(uic, cubeItems, NUM_ITEMS),
    cubeNum(BatteryLevel::BASE)
{
}

void UILowBatt::init(unsigned _cubeNum)
{
    cubeNum = _cubeNum;
    if (cubeNum == BatteryLevel::BASE)
        baseMenu.init(WARNING);
    else
        cubeMenu.init(WARNING);
}

bool UILowBatt::quitWasSelected() const
{
    if (cubeNum == BatteryLevel::BASE)
        return (static_cast<UILowBatt::Action>(baseMenu.getChosenItem()) == QUIT);
    else
        return (static_cast<UILowBatt::Action>(cubeMenu.getChosenItem()) == QUIT);
}

