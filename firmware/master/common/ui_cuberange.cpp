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
