/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "shared.h"
#include "mainmenu.h"
#include "mainmenuitem.h"
#include "defaultloadinganimation.h"
#include "assets.gen.h"
#include <sifteo.h>
#include <sifteo/menu.h>

using namespace Sifteo;

static const unsigned kFastClickAccelThreshold = 46;
static const unsigned kClickSpeedNormal = 200;
static const unsigned kClickSpeedFast = 300;
static const unsigned kDisplayBlueLogoTimeMS = 2000;
static const unsigned kNoCubesConnectedSfxMS = 5000;

static void drawText(RelocatableTileBuffer<12,12> &icon, const char *text, Int2 pos)
{
    for (int i = 0; text[i] != 0; ++i) {
        icon.image(vec(pos.x + i, pos.y), Font, text[i]-32);
    }
}

static unsigned getNumCubes(CubeSet cubes)
{
    unsigned count = 0;
    for (CubeID cube : cubes) {
        ++count;
    }
    return count;
}

const MenuAssets MainMenu::menuAssets = {
    &Menu_BgTile, &Menu_Footer, NULL, {&Menu_Tip0, &Menu_Tip1, &Menu_Tip2, NULL}
};

void MainMenu::init()
{
    Events::cubeConnect.set(&MainMenu::cubeConnect, this);
    Events::cubeDisconnect.set(&MainMenu::cubeDisconnect, this);
    Events::neighborAdd.set(&MainMenu::neighborAdded, this);
    
    time = SystemTime::now();

    items.clear();
    itemIndexCurrent = 0;
    cubeRangeSavedIcon = NULL;

    menuDirty = false;
    
    _SYS_setCubeRange(1, _SYS_NUM_CUBE_SLOTS-1); // XXX: eventually this will be the system default
}

void MainMenu::run()
{
    waitForACube();
    
    // Initialize to blue Sifteo logo to match firmware.
    for (CubeID cube : CubeSet::connected()) {
        Shared::motion[cube].attach(cube);
        Shared::video[cube].attach(cube);
        Shared::video[cube].initMode(BG0_ROM);
        Shared::video[cube].bg0.image(vec(0,0), Logo);
    }
    System::paint();
    
    AudioTracker::play(Tracker_Startup);
    
    // Pick one cube to be the 'main' cube, where we show the menu
    mainCube = *cubes().begin();

    // Load our own local assets plus all icon assets
    cubesToLoad = CubeSet::connected();
    updateAssets();

    // Run the menu on our main cube
    Menu m(Shared::video[mainCube], &menuAssets, &menuItems[0]);
    m.setIconYOffset(8);
    
    if (Volume::previous() != Volume(0)) {
        for (unsigned i = 0, e = items.count(); i != e; ++i) {
            ASSERT(items[i] != NULL);
            if (items[i]->getVolume() == Volume::previous()) {
                m.anchor(i, true);
            }
        }
    }
    
    eventLoop(m);
}

void MainMenu::eventLoop(Menu &m)
{
    int itemChoice = -1;

    struct MenuEvent e;
    while (m.pollEvent(&e)) {

        waitForACube();
        updateAnchor(m);
        updateAssets();
        updateSound(m);
        updateMusic();
        updateAlerts(m);

        bool performDefault = true;

        switch(e.type) {

            case MENU_ITEM_PRESS:
                ASSERT(e.item < arraysize(items));
                if (items[e.item]->getCubeRange().isEmpty()) {
                    AudioChannel(0).play(Sound_NonPossibleAction);
                    performDefault = false;
                } else if (!canLaunchItem(e.item)) {
                    toggleCubeRangeAlert(e.item, m);
                    performDefault = false;
                } else {
                    AudioChannel(0).play(Sound_ConfirmClick);
                    itemChoice = e.item;
                }
                break;
            case MENU_ITEM_ARRIVE:
                itemIndexCurrent = e.item;
                if (itemIndexCurrent >= 0) {
                    arriveItem(m, itemIndexCurrent);
                }
                break;
            case MENU_ITEM_DEPART:
                if (itemIndexCurrent >= 0) {
                    if (cubeRangeSavedIcon != NULL) {
                        m.replaceIcon(itemIndexCurrent, cubeRangeSavedIcon);
                        cubeRangeSavedIcon = NULL;
                    }
                    departItem(m, itemIndexCurrent);
                }
                itemIndexCurrent = -1;
                break;
            default:
                break;

        }

        if (performDefault)
            m.performDefault();
    }

    if (itemChoice != -1)
        execItem(itemChoice);
}

CubeSet MainMenu::cubes()
{
    return CubeSet::connected();
}

void MainMenu::cubeConnect(unsigned cid)
{
    AudioTracker::play(Tracker_CubeConnect);

    Shared::connectTime[cid] = SystemTime::now();
    Shared::motion[cid].attach(cid);

    Shared::video[cid].attach(cid);
    Shared::video[cid].initMode(BG0_ROM);
    Shared::video[cid].bg0.setPanning(vec(0,0));
    Shared::video[cid].bg0.image(vec(0,0), Logo);
    
    if (cid == mainCube) {
        menuDirty = true;
    }
    
    cubesToLoad.mark(cid);
}

void MainMenu::cubeDisconnect(unsigned cid)
{
    AudioTracker::play(Tracker_CubeDisconnect);

    cubesToLoad.clear(cid);
}

void MainMenu::neighborAdded(unsigned firstID, unsigned firstSide,
                             unsigned secondID, unsigned secondSide)
{
    /*
     * If a connected cube has been neighbored to the base,
     * toggle its pairing state (ie, unpair it).
     *
     * Ensure a cube has been connected to the system for at
     * least 2 seconds to avoid unpairing cubes based on a neighbor
     * event that was generated for a cube being newly paired.
     */

    CubeID cid;
    if (NeighborID(firstID).isBase())
        cid = secondID;
    else if (NeighborID(secondID).isBase())
        cid = firstID;
    else
        return;

    ASSERT(Shared::connectTime[cid].isValid());
    if (SystemTime::now() - Shared::connectTime[cid] > TimeDelta::fromMillisec(2000))
        cid.unpair();
}

void MainMenu::waitForACube()
{
    SystemTime sfxTime = SystemTime::now();
    while (CubeSet::connected().empty()) {
        if ((SystemTime::now() - sfxTime).milliseconds() >= kNoCubesConnectedSfxMS) {
            sfxTime = SystemTime::now();
            AudioChannel(0).play(Sound_NoCubesConnected);
        }
        System::yield();
    }
}

void MainMenu::updateAnchor(Menu &m)
{
    if (menuDirty) {
        m.reset();
        if (itemIndexCurrent != -1) {
            m.anchor(itemIndexCurrent);
        }
        menuDirty = false;
        System::paint();
    }
}

void MainMenu::updateAssets()
{
    if (!cubesToLoad.empty()) {
        loadAssets();
    }
}

void MainMenu::updateSound(Sifteo::Menu &menu)
{
    Sifteo::TimeDelta dt = Sifteo::SystemTime::now() - time;
    
    if (menu.getState() == MENU_STATE_TILTING) {
        unsigned threshold = abs(Shared::video[mainCube].virtualAccel().x) > kFastClickAccelThreshold ? kClickSpeedNormal : kClickSpeedFast;
        if (dt.milliseconds() >= threshold) {
            time += dt;
            AudioChannel(0).play(Sound_TiltClick);
        }
    } else if (menu.getState() == MENU_STATE_INERTIA) {
        unsigned threshold = 400;
        if (dt.milliseconds() >= threshold) {
            time += dt;
            AudioChannel(0).play(Sound_TiltClick);
        }
    }
}

void MainMenu::updateMusic()
{
    /*
     * If no other music is playing, play our menu.
     * (This lets the startup sound finish if it's still going)
     */

    if (AudioTracker::isStopped())
        AudioTracker::play(Tracker_MenuMusic);
}

bool MainMenu::canLaunchItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];

    return cubes().count() >= item->getCubeRange().sys.minCubes;
}

void MainMenu::toggleCubeRangeAlert(unsigned index, Sifteo::Menu &menu)
{
    if (cubeRangeSavedIcon == NULL) {
        AudioChannel(0).play(Sound_NonPossibleAction);

        ASSERT(index < arraysize(items));
        MainMenuItem *item = items[index];
        
        cubeRangeSavedIcon = menuItems[index].icon;
        
        cubeRangeAlertIcon.init();
        cubeRangeAlertIcon.image(vec(0,0), Icon_MoreCubes);

        String<2> buffer;
        buffer << item->getCubeRange().sys.minCubes;
        drawText(
            cubeRangeAlertIcon,
            buffer.c_str(),
            vec(item->getCubeRange().sys.minCubes < 10 ? 3 : 2, 3));
        
        unsigned numCubes = getNumCubes(CubeSet::connected());
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
        
        menu.replaceIcon(index, cubeRangeAlertIcon);
    } else {
        menu.replaceIcon(index, cubeRangeSavedIcon);
        cubeRangeSavedIcon = NULL;
    }
}

void MainMenu::updateAlerts(Sifteo::Menu &menu)
{
    // XXX: Is this really needed here?
    auto& motion = Shared::motion[mainCube];
    motion.update();

    if (cubeRangeSavedIcon != NULL && itemIndexCurrent != -1) {
        if (motion.shake || motion.tilt.x != 0 || motion.tilt.y != 0) {
            toggleCubeRangeAlert(itemIndexCurrent, menu);
        }
    }
}

void MainMenu::execItem(unsigned index)
{
    DefaultLoadingAnimation anim;

    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];

    item->getCubeRange().set();
    item->bootstrap(CubeSet::connected(), anim);
    item->exec();
}

void MainMenu::arriveItem(Menu &menu, unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->arrive(menu, index);
}

void MainMenu::departItem(Menu &menu, unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->depart(menu, index);
}

void MainMenu::loadAssets()
{
    ScopedAssetLoader loader;

    // Only load the cubes that are currently queued to load. In case there
    // is a new cube that gets connected during loading, we want to wait and
    // load that one in the next round.
    CubeSet cubesLoading = cubesToLoad;
    
    // Bind the local volume's slots.
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);

    /*
     * First, collect information about what we need to install.
     * We need to know (a) which asset groups to install, and
     * (b) whether the not-yet-installed ones will fit in our
     * slot without erasing it.
     *
     * Note that we can't really calculate this ahead-of-time,
     * since (a) depends on the state of the cubes we're using.
     *
     * To do this in a separate pass from the actual asset install,
     * we do need to map the volumes twice. Gather as much data
     * as possible now, so we can skip entire items below when
     * we know they won't require any asset loading.
     */

    unsigned uninstalledTiles = 0;
    unsigned uninstalledBytes = 0;
    unsigned totalBytes = 0;

    Volume mappedVolumes[MAX_ITEMS];

    BitArray<MAX_ITEMS> allLoadableItems;
    BitArray<MAX_ITEMS> uninstalledItems;
    allLoadableItems.clear();
    uninstalledItems.clear();

    // Clear the whole menuItems list. This zeroes the items we're
    // about to fill in, and it NULL-terminates the list.
    bzero(menuItems);

    for (unsigned I = 0, E = items.count(); I != E; ++I) {
        MainMenuItem *item = items[I];
        MappedVolume map;

        // Copy asset tiles into RAM, and map the volume
        MenuItem &mi = menuItems[I];
        auto flags = item->getAssets(mi, map);

        if (mi.icon && (flags & MainMenuItem::LOAD_ASSETS)) {
            AssetGroup &group = mi.icon->assetGroup();

            mappedVolumes[I] = map.volume();

            totalBytes += group.compressedSize();
            allLoadableItems.mark(I);

            if (!group.isInstalled(cubesLoading)) {
                uninstalledTiles += group.tileAllocation();
                uninstalledBytes += group.compressedSize();
                uninstalledItems.mark(I);
            }
        }
    }

    /*
     * Now we can calculate whether the iconSlot needs to be erased,
     * and therefore whether we're installing just the not-yet-installed
     * icon assets, or all icon assets. This lets us calculate overall
     * progress for this whole series of loading operations.
     */

    if (Shared::iconSlot.tilesFree() < uninstalledTiles) {
        // Erase the slot. Now everything is uninstalled.
        Shared::iconSlot.erase();
        uninstalledBytes = totalBytes;
        uninstalledItems = allLoadableItems;
    }

    /*
     * First asset load: Put our own menu assets into the primary slot,
     * if they aren't already there. Odds are, they will be, but this
     * lets applets repurpose the primary slot if they need to.
     */

    // Keep count of progress from previous load operations.
    DefaultLoadingAnimation anim;
    bool animStarted = false;
    unsigned progress = 0;

    if (!MenuGroup.isInstalled(cubesLoading)) {

        // Include the size of this group in our progress calculation
        uninstalledBytes += MenuGroup.compressedSize();

        if (!loader.start(MenuGroup, Shared::primarySlot, cubesLoading)) {
            Shared::primarySlot.erase();
            loader.start(MenuGroup, Shared::primarySlot, cubesLoading);
        }

        while (!loader.isComplete()) {
            if (!animStarted) {
                if (SystemTime::now().uptimeMS() >= kDisplayBlueLogoTimeMS) {
                    animStarted = true;
                    anim.begin(cubesLoading);
                }
            } else {
                for (CubeID cube : cubesLoading) {
                    anim.paint(CubeSet(cube), clamp<int>(loader.progressBytes(cube)
                        * 100 / uninstalledBytes, 0, 100));
                }
            }
            System::paint();
        }

        loader.finish();
        progress += MenuGroup.compressedSize();
    }

    /*
     * Now we can loop over the individual items that may need asset installation,
     * and install each one while calculating total progress.
     */

    for (unsigned I : uninstalledItems) {
        MappedVolume map(mappedVolumes[I]);
        MenuItem &mi = menuItems[I];

        ASSERT(mi.icon);
        AssetGroup &group = mi.icon->assetGroup();

        loader.start(group, Shared::iconSlot, cubesLoading);

        while (!loader.isComplete()) {
            if (!animStarted) {
                if (SystemTime::now().uptimeMS() >= kDisplayBlueLogoTimeMS) {
                    animStarted = true;
                    anim.begin(cubesLoading);
                }
            } else {
                for (CubeID cube : cubesLoading) {
                    anim.paint(CubeSet(cube), clamp<int>((progress + loader.progressBytes(cube))
                        * 100 / uninstalledBytes, 0, 100));
                }
            }
            System::paint();
        }

        loader.finish();
        progress += group.compressedSize();
    }

    if (animStarted) {
        anim.end(cubesLoading);
    }

    for (CubeID cube : cubesLoading) {
        // Initialize the graphics on non-menu cubes
        if (cube != mainCube) {
            auto& vid = Shared::video[cube];
            vid.initMode(BG0);
            vid.bg0.erase(Menu_StripeTile);
        }
        // Mark this cube as having been loaded
        cubesToLoad.clear(cube);
    }
}
