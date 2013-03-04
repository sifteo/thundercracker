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
    menu(uic, NULL, NUM_ITEMS)
{
}

void UILowBatt::init(uint8_t cid)
{
    ASSERT(cid <= BatteryLevel::BASE);
    const UIMenu::Item *i = (cid == BatteryLevel::BASE) ? baseItems : cubeItems;
    menu.init(WARNING, i);
}

bool UILowBatt::quitWasSelected() const
{
    return (static_cast<UILowBatt::Action>(menu.getChosenItem()) == QUIT);
}

