#include "ui_assets.h"
#include "ui_lowbatt.h"
#include "svmclock.h"
#include "svmloader.h"

const UIMenu::Item UILowBatt::items[NUM_ITEMS] = {
    // Note: labels with an even number of characters will center perfectly
    { "Base battery low", UI_IMAGE_INDEX(iconLowBattBase), true },
    { "Quit",             UI_IMAGE_INDEX(iconQuit),        true },
};

UILowBatt::UILowBatt(UICoordinator &uic) :
    menu(uic, items, NUM_ITEMS)
{
}

void UILowBatt::init()
{
    menu.init(WARNING);
}

bool UILowBatt::quitWasSelected() const
{
    return (static_cast<UILowBatt::Action>(menu.getChosenItem()) == QUIT);
}

