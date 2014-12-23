/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ui_assets.h"
#include "ui_pause.h"
#include "svmloader.h"
#include "svmclock.h"
#include "event.h"

char UIPause::gameMenuLabel[MAX_LABEL_CHARS + 1];
bool UIPause::resume;

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
    ASSERT(SvmLoader::getRunLevel() != SvmLoader::RUNLEVEL_LAUNCHER);
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
