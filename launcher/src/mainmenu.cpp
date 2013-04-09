/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
// #define MENU_LOGS_ENABLED 1
#include "shared.h"
#include "mainmenu.h"
#include "mainmenuitem.h"
#include "elfmainmenuitem.h"
#include "applet/getgames.h"
#include "applet/status.h"
#include "defaultloadinganimation.h"
#include "assets.gen.h"
#include <sifteo.h>
#include <sifteo/menu.h>

using namespace Sifteo;

static const unsigned kFastClickAccelThreshold = 46;
static const unsigned kClickSpeedNormal = 200;
static const unsigned kClickSpeedFast = 300;

static const unsigned kConnectSfxDelayMS = 2000;
static const unsigned kDisplayBlueLogoTimeMS = 2000;
static const unsigned kNoCubesConnectedSfxMS = 10000;


static void drawText(MainMenuItem::IconBuffer &icon, const char *text, Int2 pos)
{
    for (int i = 0; text[i] != 0; ++i) {
        icon.image(vec(pos.x + i, pos.y), Font, uint8_t(text[i] - ' '));
    }
}

static unsigned populate(Array<MainMenuItem*, Shared::MAX_ITEMS> &items)
{
    ELFMainMenuItem::findGames(items);
    unsigned numGames = items.count();

    GetGamesApplet::add(items);
    StatusApplet::add(items);

    return numGames;
}

const MenuAssets MainMenu::menuAssets = {
    &Menu_BgTile, &Menu_Footer, NULL, {&Menu_Tip0, &Menu_Tip1, &Menu_Tip2, NULL}
};

void MainMenu::init()
{
    // Play the startup sound as early as possible
    AudioTracker::play(Tracker_Startup);
    startupXmModHasFinished = false;
    trackerVolume = kTrackerVolumeNormal;
    isBootstrapping = false;

    Events::cubeConnect.set(&MainMenu::cubeConnect, this);
    Events::cubeDisconnect.set(&MainMenu::cubeDisconnect, this);
    Events::cubeBatteryLevelChange.set(&MainMenu::onBatteryLevelChange, this);
    Events::volumeCommit.set(&MainMenu::volumeChanged, this);
    Events::volumeDelete.set(&MainMenu::volumeChanged, this);

    loader.init();

    time = SystemTime::now();
    connectingCubes.clear();
    loadingCubes.clear();

    itemIndexCurrent = -1;
    cubeRangeSavedIcon = NULL;

    /*
     * Accommodate down to 0 cubes - this is important to avoid triggering
     * a pause due to too few cubes being connected. Plus, we want to continue
     * executing so we can give audio feedback when no cubes are around
     * to display a pause menu.
     */
    System::setCubeRange(0, _SYS_NUM_CUBE_SLOTS);
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);
}

void MainMenu::initMenu(unsigned initialIndex, bool popUp)
{
    ASSERT(items.count() > 0);

    // (Re)initialize the menu
    menu.init(Shared::video[mainCube], &menuAssets, menuItems);
    menu.setIconYOffset(8);
    menu.anchor(initialIndex, popUp);
    itemIndexCurrent = -1;
}

void MainMenu::run()
{
    init();

    numGames = populate(items);

    // Using our now-final list of items, build our AssetConfiguration
    prepareAssets();

    // Connect initial cubes
    for (CubeID cube : CubeSet::connected()) {
        cubeConnect(cube);
    }

    checkForFirstRun();

    // Start with the menu on no cube. We'll update this during the main loop.
    mainCube = CubeID();
    
    if (Volume::previous() != Volume(0)) {
        // Find a default item, based on whatever volume was running last
        for (unsigned i = 0, e = items.count(); i != e; ++i) {
            ASSERT(items[i] != NULL);
            if (items[i]->getVolume() == Volume::previous()) {
                itemIndexCurrent = i;
                break;
            }
        }

    }

    eventLoop();
}

void MainMenu::checkForFirstRun() {
    // Should we short-circuit to the first run?
    for(auto& item : items) {
        if (item->isFirstRun()) {
            MappedVolume map(item->getVolume());
            StoredObject breadcrumb(0xbc);
            int x = 0;
            if (breadcrumb.readObject(x, item->getVolume()) < 0 || x == 0) {
                while(!AudioTracker::isStopped()) {
                    System::paint();
                }
                item->exec();
            }
        }
    }


}

void MainMenu::eventLoop()
{
    while (1) {
        // Need to bind the menu to a new cube?
        if (!mainCube.isDefined()) {

            /*
             * Make sure we have at least one cube. Until we do, there's nothing
             * we can do, so just play our "cube missing" sound periodically.
             *
             * This exits as soon as at least one cube is in CubeSet::connected(),
             * but that cube may still be busy loading assets or showing the logo.
             */
            waitForACube();

            /*
             * Wait until we have a cube that's usable for our menu
             */
            while (1) {
                CubeSet usable = CubeSet::connected() & ~connectingCubes;

                if (usable.empty()) {
                    System::paint();
                    updateMusic();
                    updateConnecting();
                    System::yield();
                    continue;
                } else {
                    mainCube = *usable.begin();
                    break;
                }
            }

            if (itemIndexCurrent >= 0) {
                initMenu(itemIndexCurrent, true);
            } else {
                initMenu(0, false);
            }
        }

        MenuEvent e;
        itemIndexChoice = -1;

        // Keep running until a choice is made or the menu cube disconnects
        while (mainCube.isDefined() && menu.pollEvent(&e)) {
            updateConnecting();
            updateSound();
            updateMusic();
            updateAlerts();
            handleEvent(e);
        }

        if (itemIndexChoice >= 0) {
            return execItem(itemIndexChoice);
        }
    }
}

void MainMenu::waitForACube()
{
    SystemTime sfxTime = SystemTime::now();
    while (CubeSet::connected().empty()) {
        if ((SystemTime::now() - sfxTime).milliseconds() >= kNoCubesConnectedSfxMS) {
            sfxTime = SystemTime::now();
            AudioChannel(kConnectSoundChannel).play(Sound_NoCubesConnected);
        }
        System::yield();
    }
}

void MainMenu::handleEvent(MenuEvent &e)
{
    bool performDefault = true;

    switch (e.type) {

        case MENU_ITEM_PRESS:
            ASSERT(e.item < items.count());
            if (items[e.item]->getCubeRange().isEmpty()) {
                AudioChannel(kUIResponseSoundChannel).play(Sound_NonPossibleAction);
                performDefault = false;
            } else if (!areEnoughCubesConnected(e.item)) {
                toggleCubeRangeAlert();
                // Run the game once enough cubes have been added, or when the menu navigates away from this item.
                performDefault = false;
            } else {
                AudioChannel(kUIResponseSoundChannel).play(Sound_ConfirmClick);
                itemIndexChoice = e.item;
            }
            break;

        case MENU_ITEM_ARRIVE:
            itemIndexCurrent = e.item;
            if (itemIndexCurrent >= 0) {
                arriveItem(itemIndexCurrent);
            }
            break;

        case MENU_ITEM_DEPART:
            if (itemIndexCurrent >= 0) {
                if (cubeRangeSavedIcon != NULL) {
                    toggleCubeRangeAlert();
                }
                departItem(itemIndexCurrent);
            }
            itemIndexCurrent = -1;
            break;

        case MENU_PREPAINT:
            if (itemIndexCurrent >= 0) {
                // Detect if the applet uses the default cube responder.
                DefaultCubeResponder::resetCallCount();

                paint(itemIndexCurrent);

                // If unused, reset the responder.
                if (!DefaultCubeResponder::callCount()) {
                    for (CubeID cube : CubeSet::connected()) {
                        if (cube != menu.cube()) {
                            Shared::cubeResponder[cube].init();
                        }
                    }
                }
            } else {
                // Non-active cubes respond to events with DefaultCubeResponder.
                for (CubeID cube : CubeSet::connected()) {
                    if (cube != menu.cube()) {
                        Shared::cubeResponder[cube].paint();
                    }
                }
            }
            break;

        default:
            break;
    }

    if (performDefault)
        menu.performDefault();
}

void MainMenu::cubeConnect(unsigned cid)
{
    SystemTime now = SystemTime::now();

    AudioChannel soundChannel(kConnectSoundChannel);
    if (!soundChannel.isPlaying() && startupXmModHasFinished) {
        AudioTracker::setVolume(kTrackerVolumeDucked);
        trackerVolume = kTrackerVolumeDucked;
        soundChannel.play(Sound_CubeConnect);
    }

    if (isBootstrapping) {
        /*
         * Don't try to load the launcher's assets if we're
         * in the middle of boostrapping the game's assets.
         *
         * This should only happen if a cube disconnected
         * and reconnected during bootstrapping.
         */
        return;
    }

    // Reset this cube's connection timestamp. We won't use it until it has shown the logo for a while.
    Shared::connectTime[cid] = now;

    // This cube will be in 'connectingCubes' until it's done showing the logo and loading assets
    loadingCubes.clear(cid);
    connectingCubes.mark(cid);

    // Start loading assets
    loader.start(menuAssetConfig, CubeSet(cid));

    // Attach buffers
    Shared::motion[cid].attach(cid);
    Shared::video[cid].attach(cid);

    /*
     * Initialize to blue Sifteo logo to match firmware.
     *
     * This is in the cube's ROM, so we can show it even before asset loading.
     * TileROM images can be versioned according to the version in the hwid's
     * low byte.
     */

    uint8_t version = CubeID(cid).hwID();
    const AssetImage& logo = (version >= 0x02) ? v02_CubeConnected : v01_CubeConnected;

    Shared::video[cid].initMode(BG0_ROM);
    Shared::video[cid].bg0.setPanning(vec(0,0));
    Shared::video[cid].bg0.image(vec(0,0), logo);

    // Initialize default responders
    Shared::cubeResponder[cid].init(cid);
}

void MainMenu::cubeDisconnect(unsigned cid)
{
    AudioChannel soundChannel(kConnectSoundChannel);
    if (!soundChannel.isPlaying() && startupXmModHasFinished) {
        AudioTracker::setVolume(kTrackerVolumeDucked);
        trackerVolume = kTrackerVolumeDucked;
        soundChannel.play(Sound_CubeDisconnect);
    }

    connectingCubes.clear(cid);

    // Were we using this cube? Not any more.
    if (mainCube == cid)
        mainCube = CubeID();

    if (itemIndexCurrent >= 0) {
        ASSERT(itemIndexCurrent < items.count());
        MainMenuItem *item = items[itemIndexCurrent];
        item->onCubeDisconnect(cid);
        updateCubeRangeAlert();
    }
}

void MainMenu::onBatteryLevelChange(unsigned cid)
{
    if (itemIndexCurrent >= 0) {
        ASSERT(itemIndexCurrent < items.count());
        MainMenuItem *item = items[itemIndexCurrent];
        item->onCubeBatteryLevelChange(cid);
    }
}

void MainMenu::volumeChanged(unsigned volumeHandle)
{
    Array<MainMenuItem*, Shared::MAX_ITEMS> newItems;
    int numGamesNew = populate(newItems);
    int itemIndexNew = -1;
    bool hopUp = false;

    // If the menu is stopped at an applet (not a game), stay at that location.
    if (itemIndexCurrent >= numGames) {
        itemIndexNew = numGamesNew + numGames - itemIndexCurrent;

    } else if (itemIndexCurrent >= 0) {
        /*
         * If the menu is stopped at a game, stay at that game,
         * so long as it still exists.
         */
        for (unsigned i = 0, e = newItems.count(); i != e; ++i) {
            if (newItems[i]->getVolume() == items[itemIndexCurrent]->getVolume()) {
                itemIndexNew = i;
                break;
            }
        }

        if (itemIndexNew < 0) {
            // Return the menu to *near* where it left off,
            // but avoid going off either end of the menu.
            hopUp = true;
            itemIndexNew = clamp(itemIndexCurrent, 0, numGamesNew);
        }
    }

    // Commit the new items to the menu
    items = newItems;
    numGames = numGamesNew;

    // Update the menu
    prepareAssets();

    /*
     * In prepareAssets(), we re-init each of the menu items, which clears
     * each asset's group baseAddress, meaning it will not be valid when the menu
     * wants to draw the thumbnail, even if the AssetImage is still happily
     * installed.
     *
     * So we need to re-load everybody and reinit the menu. It's a bit of a cop out
     * to back through cubeConnect(), since we could possibly jump straight to the
     * loading animation, but this is the lowest touch route for the moment.
     */

    mainCube = CubeID::UNDEFINED;
    itemIndexCurrent = itemIndexNew;
    loader.init();

    for (CubeID cube : CubeSet::connected()) {
        cubeConnect(cube);
    }

#if 0
    /*
     * XXX: Because the menu is being reinitialized/relaunched, this will never
     * be called. This code remains in case dynamic menu item addition/removal
     * happens.
     */
    if (itemIndexCurrent >= 0) {
        ASSERT(itemIndexCurrent < items.count());
        MainMenuItem *item = items[itemIndexCurrent];
        item->onVolumeChanged(volumeHandle);
    }
#endif
}

void MainMenu::updateConnecting()
{
    /*
     * Cubes are in the 'connectingCubes' set between when they first connect
     * and when they become usable for the menu. They go through three states:
     *
     *   1. Displaying the Sifteo logo. This starts in cubeConnect(), and runs on a timer.
     *
     *   2. Loading assets. We start the load itself in cubeConnect(). After the logo
     *      finishes, we switch to displaying a progress animation.
     *
     *   3. When loading finishes, we draw an idle screen on the cube and remove it
     *      form connectingCubes.
     */

    SystemTime now = SystemTime::now();

    /*
     * Look for state transitions from (1) to (2)
     */

    CubeSet beginLoadingAnim;
    beginLoadingAnim.clear();

    for (CubeID cube : connectingCubes & ~loadingCubes) {
        if ((now - Shared::connectTime[cube]).milliseconds() >= kDisplayBlueLogoTimeMS) {
            loadingCubes.mark(cube);
            beginLoadingAnim.mark(cube);
        }
    }

    if (!beginLoadingAnim.empty()) {
        loadingAnimation.begin(beginLoadingAnim);
    }

    /*
     * Let cubes participate in the loading animation until the whole load is done
     */
    if (loadingCubes.empty()) {
        // nothing to do
        return;
    }

    if (!loader.isComplete()) {
        // Still loading, update progress
        loadingAnimation.paint(loadingCubes, loader.averageProgress(100));
        return;
    }

    // Loading is done!
    loadingAnimation.end(loadingCubes);

    // Draw an idle screen on each cube, and remove it from connectingCubes
    for (CubeID cube : loadingCubes) {
        auto& vid = Shared::video[cube];
        vid.initMode(BG0);
        vid.bg0.erase(Menu_StripeTile);
        vid.bg0.image(vec(0,0), Menu_IdleCube);

        connectingCubes.clear(cube);

        // Dispatch connected event to current applet now that the cube is ready
        if (itemIndexCurrent >= 0) {
            ASSERT(itemIndexCurrent < items.count());
            MainMenuItem *item = items[itemIndexCurrent];
            item->onCubeConnect(cube);

            // If a game was waiting on a cube to launch, try again.
            if (cubeRangeSavedIcon && areEnoughCubesConnected(itemIndexCurrent)) {
                itemIndexChoice = itemIndexCurrent;
                toggleCubeRangeAlert(); // remove the warning asking for more cubes
            }
        }

        updateCubeRangeAlert();
    }

    loadingCubes.clear();
}

void MainMenu::updateSound()
{
    if (!startupXmModHasFinished)
        return;

    Sifteo::TimeDelta dt = Sifteo::SystemTime::now() - time;

    if (menu.getState() == MENU_STATE_TILTING) {
        unsigned threshold = abs(Shared::video[mainCube].virtualAccel().x) > kFastClickAccelThreshold ? kClickSpeedNormal : kClickSpeedFast;
        if (dt.milliseconds() >= threshold) {
            time += dt;
            AudioChannel(kUIResponseSoundChannel).play(Sound_TiltClick);
        }
    }
}

void MainMenu::updateMusic()
{
    /*
     * If no other music is playing, play our menu.
     * (This lets the startup sound finish if it's still going)
     */

    if (AudioTracker::isStopped()) {
        startupXmModHasFinished = true;
        AudioTracker::play(Tracker_MenuMusic);
    }

    // if the tracker was previously ducked and the sample we were ducking from
    // has completed, bring it back up
    if (trackerVolume < kTrackerVolumeNormal &&
        !AudioChannel(kConnectSoundChannel).isPlaying())
    {
        AudioTracker::setVolume(trackerVolume++);
    }
}

bool MainMenu::areEnoughCubesConnected(unsigned index)
{
    ASSERT(index < items.count());
    MainMenuItem *item = items[index];

    return CubeSet::connected().count() >= item->getCubeRange().sys.minCubes;
}

void MainMenu::updateCubeRangeAlert()
{
    if (cubeRangeSavedIcon == NULL) return;
    if (itemIndexCurrent < 0) return;

    ASSERT(itemIndexCurrent < items.count());
    MainMenuItem *item = items[itemIndexCurrent];

    // Build a new cubeRangeAlertIcon with the updated number of cubes.
    cubeRangeAlertIcon.init();
    cubeRangeAlertIcon.image(vec(0,0), Icon_MoreCubes);

    String<2> buffer;
    buffer << item->getCubeRange().sys.minCubes;
    drawText(
        cubeRangeAlertIcon,
        buffer.c_str(),
        vec(item->getCubeRange().sys.minCubes < 10 ? 3 : 2, 3));

    unsigned numCubes = CubeSet::connected().count();
    unsigned numIcons = 12;

    if (item->getCubeRange().sys.maxCubes <= numIcons) {
        for (int i = 0; i < numIcons; ++i) {
            Int2 pos = vec((i % 4) * 2 + 2, (i / 4) * 2 + 6);
            if (i < numCubes) {
                cubeRangeAlertIcon.image(pos, MoreCubesStates, 3);
            } else if (i < item->getCubeRange().sys.minCubes) {
                cubeRangeAlertIcon.image(pos, MoreCubesStates, 2);
            } else if (i < item->getCubeRange().sys.maxCubes) {
                cubeRangeAlertIcon.image(pos, MoreCubesStates, 1);
            } else {
                cubeRangeAlertIcon.image(pos, MoreCubesStates, 0);
            }
        }
    }

    menu.replaceIcon(itemIndexCurrent, cubeRangeAlertIcon);
}

void MainMenu::toggleCubeRangeAlert()
{
    if (itemIndexCurrent < 0) {
        LOG("toggleCubeRangeAlert called when not stopped at an item!\n");
        return;
    }

    if (cubeRangeSavedIcon == NULL) {
        AudioChannel(kUIResponseSoundChannel).play(Sound_NonPossibleAction);

        ASSERT(itemIndexCurrent < items.count());
        cubeRangeSavedIcon = menuItems[itemIndexCurrent].icon;

        updateCubeRangeAlert();
    } else {
        // Restore item icon state and cancel pending launch.
        menu.replaceIcon(itemIndexCurrent, cubeRangeSavedIcon);
        cubeRangeSavedIcon = NULL;
    }
}

void MainMenu::updateAlerts()
{
    // XXX: Is this really needed here?
    auto& motion = Shared::motion[mainCube];
    motion.update();

    if (cubeRangeSavedIcon != NULL && itemIndexCurrent != -1) {
        if (motion.shake || motion.tilt.x != 0 || motion.tilt.y != 0) {
            toggleCubeRangeAlert();
        }
    }
}

void MainMenu::execItem(unsigned index)
{
    DefaultLoadingAnimation anim;

    ASSERT(index < items.count());
    MainMenuItem *item = items[index];

    item->getCubeRange().set();
    isBootstrapping = true;
    item->bootstrap(CubeSet::connected(), anim);
    item->exec();
}

void MainMenu::arriveItem(unsigned index)
{
    ASSERT(index < items.count());
    MainMenuItem *item = items[index];
    item->setMenuInfo(&menu, index);
    item->arrive();
}

void MainMenu::departItem(unsigned index)
{
    ASSERT(index < items.count());
    MainMenuItem *item = items[index];
    item->depart();
    item->setMenuInfo(NULL, -1);

    // Reset the background on all non-active cubes
    for (CubeID cube : CubeSet::connected()) {
        if (cube != menu.cube()) {
            Shared::video[cube].bg0.erase(Menu_StripeTile);
            Shared::video[cube].bg0.image(vec(0,0), Menu_IdleCube);
        }
    }
}

void MainMenu::paint(unsigned index)
{
    ASSERT(index < items.count());
    items[index]->paint();
}

void MainMenu::prepareAssets()
{
    /*
     * Gather loadable asset information from the MenuItems, and generate
     * an AssetConfiguration array.
     */

    menuAssetConfig.clear();

    // Add our local AssetGroup to the config
    menuAssetConfig.append(Shared::primarySlot, MenuGroup);

    // Clear the whole menuItems list. This zeroes the items we're
    // about to fill in, and it NULL-terminates the list.
    bzero(menuItems);

    // Set up each menu item and initialize it's state
    for (unsigned I = 0, E = items.count(); I != E; ++I) {
        items[I]->getAssets(menuItems[I], menuAssetConfig);
        items[I]->setMenuInfo(NULL, -1);
    }
}
