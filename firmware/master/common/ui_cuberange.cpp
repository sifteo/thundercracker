#include "ui_cuberange.h"
#include "svmclock.h"
#include "svmloader.h"

extern const uint16_t v01_IconQuit_data[];
extern const uint16_t v01_IconResume_data[];

const UIMenu::Item UICubeRange::items[NUM_ITEMS] = {
    // Note: labels with an odd number of characters will center perfectly
    { v01_IconResume_data,  "Connect Cubes", false },
    { v01_IconQuit_data,    "Quit Game", true },
};

UICubeRange::UICubeRange(UICoordinator &uic) :
    menu(uic, items, NUM_ITEMS)
{
}

void UICubeRange::init()
{
    ASSERT(SvmLoader::getRunLevel() != SvmLoader::RUNLEVEL_LAUNCHER);
    menu.init(CUBERANGE);
}

bool UICubeRange::quitWasSelected() const
{
    return (isDone() &&
            static_cast<UICubeRange::Action>(menu.getChosenItem()) == QUIT);
}
