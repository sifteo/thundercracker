/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "ui_assets.h"
#include "ui_pause.h"
#include "svmloader.h"
#include "svmclock.h"
#include "event.h"

char UIPause::gameMenuLabel[MAX_LABEL_CHARS + 1];

const UIMenu::Item UIPause::items[NUM_ITEMS] = {
    // Note: labels with an even number of characters will center perfectly
    { gameMenuLabel,  UI_IMAGE_INDEX(iconBack),    true },
    { "Resume",       UI_IMAGE_INDEX(iconResume),  true },
    { "Quit",         UI_IMAGE_INDEX(iconQuit),    true },
};


UIPause::UIPause(UICoordinator &uic)
    : menu(uic, &items[getFirstItem()], NUM_ITEMS - getFirstItem())
{}

void UIPause::init()
{
    menu.init(ITEM_CONTINUE - getFirstItem());
}

void UIPause::takeAction()
{
    ASSERT(!SvmClock::isPaused());

    if (isDone()) {
        switch (menu.getChosenItem() + getFirstItem()) {

            case ITEM_GAME_MENU:
                Event::setBasePending(Event::PID_BASE_GAME_MENU);
                break;

            case ITEM_CONTINUE:
                break;

            case ITEM_QUIT:
                SvmLoader::exit();
                break;

            default:
                ASSERT(0);
                break;
        }
    }
}

bool UIPause::setGameMenuLabel(SvmMemory::VirtAddr label)
{
    FlashBlockRef ref;
    if (SvmMemory::strlcpyROData(ref, gameMenuLabel, label, sizeof gameMenuLabel))
         return true;

    // Memory fault; make sure we clear the label
    disableGameMenu();
    return false;
}

void UIPause::disableGameMenu()
{
    gameMenuLabel[0] = '\0';
    ASSERT(!hasGameMenu());
}
