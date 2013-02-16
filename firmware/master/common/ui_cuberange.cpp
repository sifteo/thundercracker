#include "ui_assets.h"
#include "ui_cuberange.h"
#include "svmclock.h"
#include "svmloader.h"

const UIMenu::Item UICubeRange::items[NUM_ITEMS] = {
    // Note: labels with an even number of characters will center perfectly
    { "Connect Cube", UI_IMAGE_INDEX(iconCubeRange), false },
    { "Quit",         UI_IMAGE_INDEX(iconQuit),      true },
};

UICubeRange::UICubeRange(UICoordinator &uic) :
    menu(uic, items, NUM_ITEMS)
{
}

void UICubeRange::init()
{
    menu.init(CUBERANGE);
}

bool UICubeRange::quitWasSelected() const
{
    return (isDone() &&
            static_cast<UICubeRange::Action>(menu.getChosenItem()) == QUIT);
}
