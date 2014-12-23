/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker launcher
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

#pragma once
#include <sifteo.h>
#include <sifteo/menu.h>
#include "cuberange.h"
#include "shared.h"

class ProgressDelegate;


/**
 * Abstract base class for things that are runnable from the main menu.
 * Includes its visual representation on the menu, as well as methods for
 * bootstrapping the game (if necessary) and for transferring control to it.
 */
 
class MainMenuItem
{
public:
    typedef Sifteo::RelocatableTileBuffer<12,12> IconBuffer;

    /**
     * Retrieve pointers to the AssetImages for this menu item, and set up any
     * AssetConfiguration nodes required by the menu item. These may
     * be local assets in our own binary, or they may be copied from a game.
     */
    virtual void getAssets(Sifteo::MenuItem &assets, Shared::AssetConfiguration &config) = 0;

    /**
     * Which Volume is associated with this item, if any?
     */
    virtual Sifteo::Volume getVolume() {
        return Sifteo::Volume(0);
    }
    
    /**
     * How many cubes are required by this menu item, if any?
     */
    virtual CubeRange getCubeRange() const {
        return CubeRange();
    }

    /**
     * Is this the first-run experience?
     */
    virtual bool isFirstRun() const {
        return false;
    }

    /**
     * Bootstrap the assets necessary to launch this main menu.
     *
     * Uses the supplied ProgressDelegate to report progress during the load,
     * if a load is necessary. If no loading is necessary, returns immediately
     * without using the ProgressDelegate.
     *
     * This may switch the global AssetSlot bindings. The main menu must not
     * load any of its own assets between the calls to bootstrap() and exec().
     */
    virtual void bootstrap(Sifteo::CubeSet cubes, ProgressDelegate &progress) {}

    /**
     * Execute this menu item. This function may or may not return. If a
     * separate ELF binary is needed, we'll exec() that binary. Otherwise,
     * this performs some action and eventually returns.
     */
    virtual void exec() = 0;

    /**
     * Called by menu owner to provide a valid menu and index context for use
     * by the event hooks.
     */
    void setMenuInfo(Sifteo::Menu *m, int index) {
        menu = m;
        menuItemIndex = index;
    }

    /** 
     * Hook for menu item's arriving as the current item.
     */
    virtual void arrive() {}
    
    /** 
     * Hook for menu item's departing from being the current item.
     */
    virtual void depart() {}

    /**
     * Battery level changed event recieved while this menu item is the current item.
     */
    virtual void onCubeBatteryLevelChange(unsigned cid) {}

    /**
     * Cube connected event recieved while this menu item is the current item.
     */
    virtual void onCubeConnect(unsigned cid) {}

    /**
     * Cube disconnected event recieved while this menu item is the current item.
     */
    virtual void onCubeDisconnect(unsigned cid) {}

    /**
     * Filesystem volume changed event recieved while this menu item is the current item.
     */
    virtual void onVolumeChanged(unsigned volumeHandle) {}

    /**
     * The menu is about to paint() a frame. Use this to paint the non-active cubes.
     */
    virtual void paint() {
        ASSERT(menu);

        // Non-active cubes respond to events with DefaultCubeResponder by default.
        for (Sifteo::CubeID cube : Sifteo::CubeSet::connected()) {
            if (cube != menu->cube()) {
                Shared::cubeResponder[cube].paint();
            }
        }
    }

protected:
    Sifteo::Menu *menu;
    int menuItemIndex;
};
