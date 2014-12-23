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

#ifndef _UI_PAUSE_H
#define _UI_PAUSE_H

#include "ui_menu.h"
#include "svmmemory.h"

/**
 * UIPause is the system UI we display when a game is paused.
 *
 * It includes options for exiting the current game or resuming
 * it, and an optional choice for a game-specific menu, with
 * a custom label string.
 */

class UIPause {
public:
    static const unsigned MAX_LABEL_CHARS = 15;

    UIPause(UICoordinator &uic);

    void init();
    void takeAction();

    static bool setGameMenuLabel(SvmMemory::VirtAddr label);
    static void disableGameMenu();
    static void setResumeEnabled(bool enabled) {
        resume = enabled;
    }

    ALWAYS_INLINE void animate() {
        return menu.animate();
    }

    ALWAYS_INLINE bool isDone() const {
        return menu.isDone();
    }

    static ALWAYS_INLINE bool hasGameMenu() {
        return gameMenuLabel[0] != 0;
    }

    static ALWAYS_INLINE bool resumeEnabled() {
        return resume;
    }

private:
    UIMenu menu;

    enum Item {
        ITEM_GAME_MENU = 0,
        ITEM_CONTINUE,
        ITEM_QUIT,
        NUM_ITEMS
    };

    static ALWAYS_INLINE unsigned getFirstItem() {

        /*
         * XXX: This is slightly half-baked, but if a game menu is requested
         *      and resume is disabled, resume will still be rendered.
         *
         *      This is OK for now, since the only real requirement for
         *      suppressing 'resume' is the first run app, which does not
         *      require a game menu.
         */

        if (hasGameMenu()) {
            return ITEM_GAME_MENU;
        }

        if (resumeEnabled()) {
            return ITEM_CONTINUE;
        }

        return ITEM_QUIT;
    }

    static char gameMenuLabel[MAX_LABEL_CHARS + 1];
    static bool resume;
    static const UIMenu::Item items[NUM_ITEMS];
};


#endif
