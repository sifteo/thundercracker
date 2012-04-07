#include <sifteo.h>
#include "assets.gen.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "EventID.h"
#include "EventData.h"
#include "menu.h"

using namespace Sifteo;

static const char* sideNames[] =
{
  "top", "left", "bottom", "right"  
};

static struct MenuItem gItems[] =
{ {&IconContinue, &LabelContinue},
  {&IconLocked1, &LabelLocked},
  {&IconLocked2, &LabelLocked},
  {&IconLocked3, &LabelLocked},
  {&IconLocked0, &LabelLocked},
  {&IconLocked1, &LabelLocked},
  {&IconLocked2, &LabelLocked},
  {&IconLocked3, &LabelLocked},
  {NULL, NULL} };

static struct MenuAssets gAssets =
{&BgTile, &Footer, &LabelEmpty, {&Tip0, &Tip1, &Tip2, NULL}};

void onCubeEventTouch(void *context, _SYSCubeID cid)
{
    DEBUG_LOG(("cube event touch:\t%d\n", cid));
    // TODO TouchAndHold timer
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Touch, data);

#ifdef DEBUGzz
    DEBUG_LOG(("cube event touch->shake, ID:\t%d\n", cid));
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Shake, data);
#endif
}

void onCubeEventShake(void *context, _SYSCubeID cid)
{
    DEBUG_LOG(("cube event shake, ID:\t%d\n", cid));
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Shake, data);
}

void onCubeEventTilt(void *context, _SYSCubeID cid)
{
    DEBUG_LOG(("cube event tilt:\t%d\n", cid));
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Tilt, data);
}

void onNeighborEventAdd(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    EventData data;
    WordGame::onEvent(EventID_AddNeighbor, data);
    LOG(("neighbor add:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void onNeighborEventRemove(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    EventData data;
    WordGame::onEvent(EventID_RemoveNeighbor, data);
    LOG(("neighbor remove:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void accel(_SYSCubeID c)
{
    DEBUG_LOG(("accelerometer changed\n"));
}

void siftmain()
{
    DEBUG_LOG(("Hello, Word Caravan\n"));

    _SYS_setVector(_SYS_CUBE_TOUCH, (void*) onCubeEventTouch, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*) onCubeEventShake, NULL);
    _SYS_setVector(_SYS_CUBE_TILT, (void*) onCubeEventTilt, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborEventAdd, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighborEventRemove, NULL);

    static Cube cubes[NUM_CUBES]; // must be static!

    for (unsigned i = 0; i < arraysize(cubes); i++)
    {
        cubes[i].enable(i + CUBE_ID_BASE);
    }

#ifndef DEBUGz
    if (0 && LOAD_ASSETS)
    {
        // start loading assets
        for (unsigned i = 0; i < arraysize(cubes); i++)
        {
            cubes[i].loadAssets(GameAssets);

            VidMode_BG0_ROM rom(cubes[i].vbuf);
            rom.init();

            rom.BG0_text(Vec2(1,1), "Loading...");
        }

        // wait for assets to finish loading
        for (;;)
        {
            bool done = true;

            for (unsigned i = 0; i < arraysize(cubes); i++)
            {
                VidMode_BG0_ROM rom(cubes[i].vbuf);
                rom.BG0_progressBar(Vec2(0,7),
                                    cubes[i].assetProgress(GameAssets,
                                                           VidMode_BG0_SPR_BG1::LCD_width),
                                    2);

                if (!cubes[i].assetDone(GameAssets))
                {
                    done = false;
                }
            }

            System::paint();

            if (done)
            {
                break;
            }
        }


    }
#endif // ifndef DEBUG

    // sync up
    System::paintSync();
    for(Cube* p=cubes; p!=cubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
    System::paintSync();
    for(Cube* p=cubes; p!=cubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
    System::paintSync();

    Cube *pMenuCube = &cubes[0];
    Menu m(pMenuCube, &gAssets, gItems);

    // main loop
    WordGame game(cubes, pMenuCube, m); // must not be static!
    // TODO use clockNS, to avoid precision bugs with long play sessions
    float lastTime = System::clock();
    float lastPaint = System::clock();
    while (1)
    {
        float now = System::clock();
        float dt = now - lastTime;
        lastTime = now;

        game.update(dt);

        // decouple paint frequency from update frequency
        if (now - lastPaint >= 1.f/25.f)
        {
            game.onEvent(EventID_Paint, EventData());
            lastPaint = now;
        }

        if (true || game.needsPaintSync()) // TODO can't seem to fix BG1 weirdness w/o this
        {
            game.paintSync();
        }
        else
        {
            System::paint(); // (will do nothing if screens haven't changed
        }
    }
}

/*  The __cxa_pure_virtual function is an error handler that is invoked when a
    pure virtual function is called. If you are writing a C++ application that
    has pure virtual functions you must supply your own __cxa_pure_virtual
    error handler function. For example:
*/
extern "C" void __cxa_pure_virtual() { while (1); }



void assertWrapper(bool testResult)
{
#ifdef SIFTEO_SIMULATOR
    if (!testResult)
    {
        assert(testResult);
    }
#endif
}





// constant folding
const float Menu::kOneG = abs(64 * kAccelScalingFactor);

Menu::Menu(Cube *mainCube, struct MenuAssets *aAssets, struct MenuItem *aItems)
    : canvas(mainCube->vbuf) {
    kHeaderHeight = 0;
    kFooterHeight = 0;
    kFooterBG1Offset = 0;
    kIconYOffset = 0;
    kIconTileWidth = 0;
    kIconTileHeight = 0;
    kEndCapPadding = 0;
    pCube = 0;				// cube on which the menu is being drawn
    assets = 0;	// theme assets of the menu
    numTips = 0;			// number of tips in the theme
    numItems = 0;			// number of items in the strip
    // event breadcrumb
    _SYS_memset8((uint8_t*)&currentEvent, 0, sizeof(currentEvent));
    // state tracking
    currentState = (MenuState)0;
    stateFinished = false;
    // paint hinting
    shouldPaintSync = false;
    // accelerometer caching
    xaccel = 0.f;
    yaccel  = 0.f;
    // footer drawing
    currentTip = 0;
    prevTipTime  = 0.f;
    // static state: event at beginning of touch only
    prevTouch = false;
    // inertial state: where to stop
    stopping_position = 0;
    tiltDirection = 0;
    // scrolling states (Inertia and Tilt): physics
    position = 0.f;			// current x position
    prev_ut = 0;			// tile validity tracker
    velocity  = 0.f;			// current velocity
    frameclock  = 0.f;	// framerate timer
    frameclockPrev  = 0.f;	// framerate timer
    dt  = 0.f;	// framerate timer
    // finish state: animation iterations
    finishIteration = 0;

    currentEvent.type = MENU_UNEVENTFUL;
    pCube = mainCube;
    changeState(MENU_STATE_START);
    currentEvent.type = MENU_UNEVENTFUL;
    items = aItems;
    assets = aAssets;


    // calculate the number of items
    uint8_t i = 0;
    while(items[i].icon != NULL) {
        if (kIconTileWidth == 0) {
            kIconTileWidth = items[i].icon->width;
            kIconTileHeight = items[i].icon->height;
            kEndCapPadding = (kNumVisibleTilesX - kIconTileWidth) * (kPixelsPerTile / 2.f);
            ASSERT((kItemPixelWidth - kIconPixelWidth) % kPixelsPerTile == 0);
            // icons should leave at least one tile on both sides for the next/prev items
            ASSERT(kIconTileWidth <= kNumVisibleTilesX - (kPeekTiles * 2));
        } else {
            ASSERT(items[i].icon->width == kIconTileWidth);
            ASSERT(items[i].icon->height == kIconTileHeight);
        }

        i++;
        if (items[i].label != NULL) {
            ASSERT(items[i].label->width == kNumVisibleTilesX);
            if (kHeaderHeight == 0) {
                kHeaderHeight = items[i].label->height;
            } else {
                ASSERT(items[i].label->height == kHeaderHeight);
            }
            /* XXX: if there are any labels, a header is required for now.
             * supporting labels and no header would require toggling bg0
             * tiles fairly often and that's state I don't want to deal with
             * for the time being.
             * workaround: header can be an appropriately-sized, entirely
             * transparent PNG.
             */
            ASSERT(assets->header != NULL);
        }
    }
    numItems = i;

    // calculate the number of tips
    i = 0;
    while(assets->tips[i] != NULL) {
        ASSERT(assets->tips[i]->width == kNumVisibleTilesX);
        if (kFooterHeight == 0) {
            kFooterHeight = assets->tips[i]->height;
        } else {
            ASSERT(assets->tips[i]->height == kFooterHeight);
        }
        i++;
    }
    numTips = i;

    // sanity check the rest of the assets
    ASSERT(assets->background);
    ASSERT(assets->background->width == 1 && assets->background->height == 1);
    if (assets->footer) {
        ASSERT(assets->footer->width == kNumVisibleTilesX);
        if (kFooterHeight == 0) {
            kFooterHeight = assets->footer->height;
        } else {
            ASSERT(assets->footer->height == kFooterHeight);
        }
    }

    if (assets->header) {
        ASSERT(assets->header->width == kNumVisibleTilesX);
        if (kHeaderHeight == 0) {
            kHeaderHeight = assets->header->height;
        } else {
            ASSERT(assets->header->height == kHeaderHeight);
        }
    }
    kFooterBG1Offset = assets->header == NULL ? 0 : kNumVisibleTilesX * kHeaderHeight;

    setIconYOffset(kDefaultIconYOffset);
}

/*
 *              _     _ _                         _   _
 *  _ __  _   _| |__ | (_) ___   _ __ _   _ _ __ | |_(_)_ __ ___   ___
 * | '_ \| | | | '_ \| | |/ __| | '__| | | | '_ \| __| | '_ ` _ \ / _ \
 * | |_) | |_| | |_) | | | (__  | |  | |_| | | | | |_| | | | | | |  __/
 * | .__/ \__,_|_.__/|_|_|\___| |_|   \__,_|_| |_|\__|_|_| |_| |_|\___|
 * |_|
 *
 */

bool Menu::pollEvent(struct MenuEvent *ev) {
    // handle/clear pending events
    if (currentEvent.type != MENU_UNEVENTFUL) {
        performDefault();
    }
    /* state changes can happen in the default event handler which may dispatch
     * events (like MENU_STATE_STATIC -> MENU_STATE_FINISH dispatches a
     * MENU_PREPAINT).
     */
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // keep track of time so if our framerate changes, apparent speed persists
    frameclockPrev = frameclock;
    frameclock = System::clock();
    dt = frameclock - frameclockPrev;

    // neighbor changes?
    if (currentState != MENU_STATE_START) {
        detectNeighbors();
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }
    // update commonly-used data
    shouldPaintSync = false;
    const float kAccelScalingFactor = -0.25f;
    xaccel = kAccelScalingFactor * pCube->virtualAccel().x;
    yaccel = kAccelScalingFactor * pCube->virtualAccel().y;

    // state changes
    switch(currentState) {
        case MENU_STATE_START:
            transFromStart();
            break;
        case MENU_STATE_STATIC:
            transFromStatic();
            break;
        case MENU_STATE_TILTING:
            transFromTilting();
            break;
        case MENU_STATE_INERTIA:
            transFromInertia();
            break;
        case MENU_STATE_FINISH:
            transFromFinish();
            break;
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // run loop
    switch(currentState) {
        case MENU_STATE_START:
            stateStart();
            break;
        case MENU_STATE_STATIC:
            stateStatic();
            break;
        case MENU_STATE_TILTING:
            stateTilting();
            break;
        case MENU_STATE_INERTIA:
            stateInertia();
            break;
        case MENU_STATE_FINISH:
            stateFinish();
            break;
    }
    if (dispatchEvent(ev)) {
        return (ev->type != MENU_EXIT);
    }

    // no special events, paint a frame.
    drawFooter();
    currentEvent.type = MENU_PREPAINT;
    dispatchEvent(ev);
    return true;
}

void Menu::preventDefault() {
    /* paints shouldn't be prevented because:
     * the caller doesn't know whether to paintSync or paint and shouldn't be
     * painting while the menu owns the context.
     */
    ASSERT(currentEvent.type != MENU_PREPAINT);
    /* exit shouldn't be prevented because:
     * the default handler is responsible for resetting the menu if the event
     * pump is restarted.
     */
    ASSERT(currentEvent.type != MENU_EXIT);

    clearEvent();
}

void Menu::reset() {
    changeState(MENU_STATE_START);
}

void Menu::replaceIcon(uint8_t item, const AssetImage *icon) {
    ASSERT(item < numItems);
    items[item].icon = icon;

    for(int i = prev_ut; i < prev_ut + kNumTilesX; i++)
        if (itemVisibleAtCol(item, i))
            drawColumn(i);
}

void Menu::paintSync() {
    // this is only relevant when caller is handling a PREPAINT event.
    ASSERT(currentEvent.type == MENU_PREPAINT);
    shouldPaintSync = true;
}

bool Menu::itemVisible(uint8_t item) {
    ASSERT(item >= 0 && item < numItems);

    for(int i = MAX(0, prev_ut); i < prev_ut + kNumTilesX; i++) {
        if (itemVisibleAtCol(item, i)) return true;
    }
    return false;
}

void Menu::setIconYOffset(uint8_t px) {
    ASSERT(px >= 0 || px < kNumTilesX * 8);
    kIconYOffset = -px;
    updateBG0();
}

/*
 *      _        _         _                     _ _ _
 *  ___| |_ __ _| |_ ___  | |__   __ _ _ __   __| | (_)_ __   __ _
 * / __| __/ _` | __/ _ \ | '_ \ / _` | '_ \ / _` | | | '_ \ / _` |
 * \__ \ || (_| | ||  __/ | | | | (_| | | | | (_| | | | | | | (_| |
 * |___/\__\__,_|\__\___| |_| |_|\__,_|_| |_|\__,_|_|_|_| |_|\__, |
 *                                                           |___/
 *
 * States are represented by three functions per state:
 * transTo$TATE() - called whenever transitioning into the state.
 * state$TATE() - the state itself, called every loop of the event pump.
 * transFrom$TATE() - called before every loop of the event pump, responsible
 *                    for transitioning to other states.
 *
 * States are identified by MENU_STATE_$TATE values in enum MenuState, and are
 * mapped to their respective functions in changeState and pollEvent.
 */

void Menu::changeState(MenuState newstate) {
    stateFinished = false;
    currentState = newstate;

    //DEBUG_LOG(("STATE: -> "));
    switch(currentState) {
        case MENU_STATE_START:
            //DEBUG_LOG(("start\n"));
            transToStart();
            break;
        case MENU_STATE_STATIC:
            //DEBUG_LOG(("static\n"));
            transToStatic();
            break;
        case MENU_STATE_TILTING:
            //DEBUG_LOG(("tilting\n"));
            transToTilting();
            break;
        case MENU_STATE_INERTIA:
            //DEBUG_LOG(("inertia\n"));
            transToInertia();
            break;
        case MENU_STATE_FINISH:
            //DEBUG_LOG(("finish\n"));
            transToFinish();
            break;
    }
}

/* Start state: MENU_STATE_START
 * This state is responsible for initialization of a potentially dirty menu
 * object, including setting up the video modes and animating the menu in.
 * Transitions:
 * -> Static when initialization is complete and menu becomes interactive.
 * Events:
 * none.
 */
void Menu::transToStart() {
    shouldPaintSync = true;
    handlePrepaint();
}

void Menu::stateStart() {
    // initialize video state
    canvas.clear();
    for(unsigned r=0; r<18; ++r)
        for(unsigned c=0; c<18; ++c)
            canvas.BG0_drawAsset(Vec2(c,r), *assets->background);

    canvas.BG1_setPanning(Vec2(0, 0));
    BG1Helper(*pCube).Flush();
    {
        // Allocate tiles for the static upper label, and draw it.
        if (kHeaderHeight) {
            const AssetImage& label = items[0].label ? *items[0].label : *assets->header;
            _SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, ((1 << label.width) - 1), label.height);
            _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
        }

        // Allocate tiles for the footer, and draw it.
        if (assets->footer) {
            const AssetImage& footer = *assets->footer;
            _SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + (kNumVisibleTilesY - footer.height), ((1 << footer.width) - 1), footer.height);
            _SYS_vbuf_writei(
                &pCube->vbuf.sys,
                offsetof(_SYSVideoRAM, bg1_tiles) / 2 + kFooterBG1Offset,
                footer.tiles,
                0,
                footer.width * footer.height
            );
        }
    }

    currentTip = 0;
    prevTipTime = System::clock();
    drawFooter(true);

    canvas.set();

    shouldPaintSync = true;

    // if/when we start animating the menu into existence, set this once the work is complete
    stateFinished = true;
}

void Menu::transFromStart() {
    if (stateFinished) {
        position = 0.f;
        prev_ut = computeCurrentTile() + kNumTilesX;
        updateBG0();

        for(int i = 0; i < NUM_SIDES; i++) {
            neighbors[i].neighborSide = SIDE_UNDEFINED;
            neighbors[i].neighbor = CUBE_ID_UNDEFINED;
            neighbors[i].masterSide = SIDE_UNDEFINED;
        }

        updateBG0();
        changeState(MENU_STATE_STATIC);
    }
}

/* Static state: MENU_STATE_STATIC
 * The resting state of the menu, the MENU_ITEM_PRESS event is fired from this
 * state on a new touch event.
 * Transitions:
 * -> Tilting when x accelerometer exceeds kAccelThresholdOn.
 * Events:
 * MENU_ITEM_ARRIVE fired when entering this state.
 * MENU_ITEM_PRESS fired when cube is touched in this state.
 * MENU_ITEM_DEPART fired when leaving this state due to tilt.
 */
void Menu::transToStatic() {
    velocity = 0;
    prevTouch = pCube->touching();

    currentEvent.type = MENU_ITEM_ARRIVE;
    currentEvent.item = computeSelected();

    // show the title of the item
    if (kHeaderHeight) {
        const AssetImage& label = items[currentEvent.item].label ? *items[currentEvent.item].label : *assets->header;
        _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
    }
}

void Menu::stateStatic() {
    bool touch = pCube->touching();

    if (touch && !prevTouch) {
        currentEvent.type = MENU_ITEM_PRESS;
        currentEvent.item = computeSelected();
    }
    prevTouch = touch;
}

void Menu::transFromStatic() {
    if (abs(xaccel) > kAccelThresholdOn) {
        changeState(MENU_STATE_TILTING);

        currentEvent.type = MENU_ITEM_DEPART;
        currentEvent.item = computeSelected();

        // hide header
        if (kHeaderHeight) {
            const AssetImage& label = *assets->header;
            _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
        }
    }
}

/* Tilting state: MENU_STATE_TILTING
 * This state is active when the menu is being scrolled due to tilt and has not
 * yet hit a backstop.
 * Transitions:
 * -> Inertia when scroll goes out of bounds or cube is no longer tilted > kAccelThresholdOff.
 * Events:
 * none.
 */
void Menu::transToTilting() {
    ASSERT(abs(xaccel) > kAccelThresholdOn);
}

void Menu::stateTilting() {
    // normal scrolling
    const int max_x = stoppingPositionFor(numItems - 1);
    const float kInertiaThreshold = 10.f;

    velocity += (xaccel * dt * kTimeDilator) * velocityMultiplier();

    // clamp maximum velocity based on cube angle
    if (abs(velocity) > maxVelocity()) {
        velocity = (velocity < 0 ? 0 - maxVelocity() : maxVelocity());
    }

    // don't go past the backstop unless we have inertia
    if ((position > 0.f && velocity < 0) || (position < max_x && velocity > 0) || abs(velocity) > kInertiaThreshold) {
        position += velocity * dt * kTimeDilator;
    } else {
        velocity = 0;
    }
    updateBG0();
}

void Menu::transFromTilting() {
    const bool outOfBounds = (position < -0.05f) || (position > kItemPixelWidth*(numItems-1) + kEndCapPadding + 0.05f);
    if (abs(xaccel) < kAccelThresholdOff || outOfBounds) {
        changeState(MENU_STATE_INERTIA);
    }
}

/* Inertia state: MENU_STATE_INERTIA
 * This state is active when the menu is scrolling but either by inertia or by
 * scrolling past the backstop.
 * Transitions:
 * -> Tilting when x accelerometer exceeds kAccelThresholdOn.
 * -> Static when menu arrives at its stopping position.
 * Events:
 * none.
 */
void Menu::transToInertia() {
    stopping_position = stoppingPositionFor(computeSelected());
    if (abs(xaccel) > kAccelThresholdOff) {
        tiltDirection = (kAccelScalingFactor * xaccel < 0) ? 1 : -1;
    } else {
        tiltDirection = 0;
    }
}

void Menu::stateInertia() {
    const float stiffness = 0.333f;

    // do not pull to item unless tilting has stopped.
    if (abs(xaccel) < kAccelThresholdOff) {
        tiltDirection = 0;
    }
    // if still tilting, do not bounce back to the stopping position.
    if ((tiltDirection < 0 && velocity >= 0.f) || (tiltDirection > 0 && velocity <= 0.f)) {
            return;
    }

    velocity += stopping_position - position;
    velocity *= stiffness;
    position += velocity * dt * kTimeDilator;
    position = lerp(position, stopping_position, 0.15f);

    stateFinished = abs(velocity) < 1.0f && abs(stopping_position - position) < 0.5f;
    if (stateFinished) {
        // prevent being off by one pixel when we stop
        position = stopping_position;
    }

    updateBG0();
}

void Menu::transFromInertia() {
    if (abs(xaccel) > kAccelThresholdOn &&
        !((tiltDirection < 0 && xaccel < 0.f) || (tiltDirection > 0 && xaccel > 0.f))) {
        changeState(MENU_STATE_TILTING);
    }
    if (stateFinished) { // stateFinished formerly doneTilting
        changeState(MENU_STATE_STATIC);
    }
}

/* Finish state: MENU_STATE_FINISH
 * This state is responsible for animating the menu out.
 * Transitions:
 * -> Start when finished and state is run an extra time. This resets and
 *          re-enters the menu.
 * Events:
 * MENU_EXIT fired after the last iteration has been painted, indicating that
 *           the menu is finished and the caller should leave its event loop.
 */
void Menu::transToFinish() {
    // prepare screen for item animation
    // isolate the selected icon
    canvas.BG0_setPanning(Vec2(0, 0));

    // blank out the background layer
    for(int row=0; row<kIconTileHeight; ++row)
    for(int col=0; col<kNumTilesX; ++col) {
        canvas.BG0_drawAsset(Vec2(col, row), *assets->background);
    }
    if (assets->footer) {
        Vec2 vec( 0, kNumVisibleTilesY - assets->footer->height );
        canvas.BG0_drawAsset(vec, *assets->footer);
    }
    {
        const AssetImage* icon = items[computeSelected()].icon;
        BG1Helper overlay(*pCube);
        Vec2 vec(0, 0);
        overlay.DrawAsset(vec, *icon);
        overlay.Flush();
    }
    finishIteration = 0;
    shouldPaintSync = true;
    currentEvent.type = MENU_PREPAINT;
}

void Menu::stateFinish() {
    const float k = 5.f;
    int offset = 0;

    finishIteration++;
    float u = finishIteration/33.f;
    u = (1.f-k*u);
    offset = int(12*(1.f-u*u));
    canvas.BG1_setPanning(Vec2(-kEndCapPadding, offset + kIconYOffset));
    currentEvent.type = MENU_PREPAINT;

    if (offset <= -128) {
        currentEvent.type = MENU_EXIT;
        currentEvent.item = computeSelected();
        stateFinished = true;
    }
}

void Menu::transFromFinish() {
    if (stateFinished) {
        // We already animated ourselves out of a job. If we're being called again, reset the menu
        changeState(MENU_STATE_START);

        // And re-run the first iteraton of the event loop
        MenuEvent ignore;
        pollEvent(&ignore);
        /* currentEvent will be set by this second iteration of pollEvent,
         * so when we return from this function the currentEvent will be
         * propagated back to pollEvent's parameter.
         */
    }
}

/*
 *                       _     _                     _ _ _
 *   _____   _____ _ __ | |_  | |__   __ _ _ __   __| | (_)_ __   __ _
 *  / _ \ \ / / _ \ '_ \| __| | '_ \ / _` | '_ \ / _` | | | '_ \ / _` |
 * |  __/\ V /  __/ | | | |_  | | | | (_| | | | | (_| | | | | | | (_| |
 *  \___| \_/ \___|_| |_|\__| |_| |_|\__,_|_| |_|\__,_|_|_|_| |_|\__, |
 *                                                               |___/
 *
 * Stateful events are dispatched by setting MenuEvent.type (and
 * MenuEvent.neighbor/MenuEvent.item as appropriate) in a transTo$TATE or
 * state$TATE method and returning immediately.
 *
 * All events are immediately caught by the dispatchEvent method in pollEvent,
 * which copies it into the caller's event structure, and returns control to
 * the caller to handle the event.
 *
 * If the caller would like to prevent the default handling of the event,
 * calling Menu::preventDefault will clear the event. This is useful for
 * multistate items (the MENU_ITEM_PRESS event can replace the item's icon
 * using replaceIcon to reflect the new state instead of animating out of the
 * menu).
 * NOTE: MENU_PREPAINT should never have its default behaviour disabled as the
 * menu tracks internally if a synchronous paint is needed. If the caller needs
 * a synchronous paint for the other cubes in the system, Menu::paintSync will
 * force a synchronous paint for the current paint operation.
 *
 * Default event handling is performed at the beginning of pollEvent, before
 * re-entering the state machine.
 */

void Menu::handleNeighborAdd() {
    //DEBUG_LOG(("Default handler: neighborAdd\n"));
    // TODO: play a sound
}

void Menu::handleNeighborRemove() {
    //DEBUG_LOG(("Default handler: neighborRemove\n"));
    // TODO: play a sound
}

void Menu::handleItemArrive() {
    //DEBUG_LOG(("Default handler: itemArrive\n"));
    // TODO: play a sound
}

void Menu::handleItemDepart() {
    //DEBUG_LOG(("Default handler: itemDepart\n"));
    // TODO: play a sound
}

void Menu::handleItemPress() {
    //DEBUG_LOG(("Default handler: itemPress\n"));
    // animate out icon
    changeState(MENU_STATE_FINISH);
}

void Menu::handleExit() {
    //DEBUG_LOG(("Default handler: exit\n"));
    // nothing
}

void Menu::handlePrepaint() {
    if (shouldPaintSync) {
        // GFX Workaround
        System::paintSync();
        pCube->vbuf.touch();
        System::paintSync();
    } else {
        System::paint();
    }
}

void Menu::performDefault() {
    switch(currentEvent.type) {
        case MENU_NEIGHBOR_ADD:
            handleNeighborAdd();
            break;
        case MENU_NEIGHBOR_REMOVE:
            handleNeighborRemove();
            break;
        case MENU_ITEM_ARRIVE:
            handleItemArrive();
            break;
        case MENU_ITEM_DEPART:
            handleItemDepart();
            break;
        case MENU_ITEM_PRESS:
            handleItemPress();
            break;
        case MENU_EXIT:
            handleExit();
            break;
        case MENU_PREPAINT:
            handlePrepaint();
            break;
        default:
            break;
    }

    // clear event
    clearEvent();
}

void Menu::clearEvent() {
    currentEvent.type = MENU_UNEVENTFUL;
}

bool Menu::dispatchEvent(struct MenuEvent *ev) {
    if (currentEvent.type != MENU_UNEVENTFUL) {
        *ev = currentEvent;
        return true;
    }
    return false;
}

/*
 *  _ _ _
 * | (_) |__  _ __ __ _ _ __ _   _
 * | | | '_ \| '__/ _` | '__| | | |
 * | | | |_) | | | (_| | |  | |_| |
 * |_|_|_.__/|_|  \__,_|_|   \__, |
 *                           |___/
 */

void Menu::detectNeighbors() {
    for(_SYSSideID i = 0; i < NUM_SIDES; i++) {
        MenuNeighbor n;
        if (pCube->hasPhysicalNeighborAt(i)) {
            Cube c(pCube->physicalNeighborAt(i));
            n.neighborSide = c.physicalSideOf(pCube->id());
            n.neighbor = c.id();
            n.masterSide = i;
        } else {
            n.neighborSide = SIDE_UNDEFINED;
            n.neighbor = CUBE_ID_UNDEFINED;
            n.masterSide = SIDE_UNDEFINED;
        }

        if (n != neighbors[i]) {
            // Neighbor was set but is now different/gone
            if (neighbors[i].neighbor != CUBE_ID_UNDEFINED) {
                // Generate a lost neighbor event, with the ghost of the lost cube.
                currentEvent.type = MENU_NEIGHBOR_REMOVE;
                currentEvent.neighbor = neighbors[i];

                /* Act as if the neighbor was lost, even if it wasn't. We'll
                 * synthesize another event on the next run of the event loop
                 * if the neighbor was replaced by another cube in less than
                 * one polling cycle.
                 */
                neighbors[i].neighborSide = SIDE_UNDEFINED;
                neighbors[i].neighbor = CUBE_ID_UNDEFINED;
                neighbors[i].masterSide = SIDE_UNDEFINED;
            } else {
                neighbors[i] = n;
                currentEvent.type = MENU_NEIGHBOR_ADD;
                currentEvent.neighbor = n;
            }

            /* We don't have a method of queueing events, so return as soon as
             * a change is discovered. If there are other changes, the next run
             * of the event loop will discover them.
             */
            return;
        }
    }
}

uint8_t Menu::computeSelected() {
    int s = (position + (kItemPixelWidth / 2)) / kItemPixelWidth;
    return clamp(s, 0, numItems - 1);
}

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
void Menu::drawColumn(int x) {
    // x is the column in "global" space
    uint16_t addr = unsignedMod(x, kNumTilesX);

    // icon or blank column?
    if (itemAtCol(x) < numItems) {
        // drawing an icon column
        const AssetImage* pImg = items[itemAtCol(x)].icon;
        const uint16_t *src = pImg->tiles + ((x % kItemTileWidth) % pImg->width);

        for(int row=0; row<kIconTileHeight; ++row) {
            _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
            addr += kNumTilesX;
            src += pImg->width;

        }
    } else {
        // drawing a blank column
        for(int row=0; row<kIconTileHeight; ++row) {
            Vec2 vec( addr, row );
            canvas.BG0_drawAsset(vec, *assets->background);
        }
    }
}

unsigned Menu::unsignedMod(int x, unsigned y) {
    const int z = x % (int)y;
    return z < 0 ? z+y : z;
}

void Menu::drawFooter(bool force) {
    const AssetImage& footer = numTips > 0 ? *assets->tips[currentTip] : *assets->footer;
    const float kSecondsPerTip = 4.f;

    if (numTips == 0 || assets->footer == NULL) return;

    if (System::clock() - prevTipTime > kSecondsPerTip || force) {
        prevTipTime = System::clock();

        if (numTips > 0) {
            currentTip = (currentTip+1) % numTips;
        }

        _SYS_vbuf_writei(
            &pCube->vbuf.sys,
            offsetof(_SYSVideoRAM, bg1_tiles) / 2 + kFooterBG1Offset,
            footer.tiles,
            0,
            footer.width * footer.height
        );
    }
}

int Menu::stoppingPositionFor(int selected) {
    return kItemPixelWidth * selected;
}

float Menu::velocityMultiplier() {
    return yaccel > kAccelThresholdOff ? (1.f + (yaccel * kMaxSpeedMultiplier / kOneG)) : 1.f;
}

float Menu::maxVelocity() {
    const float kMaxNormalSpeed = 40.f;
    return kMaxNormalSpeed *
           // x-axis linear limit
           (abs(xaccel) / kOneG) *
           // y-axis multiplier
           velocityMultiplier();
}

float Menu::lerp(float min, float max, float u) {
    return min + u * (max - min);
}

void Menu::updateBG0() {
    int ut = computeCurrentTile();

    /* XXX: we should always paintSync if we've loaded two columns, to avoid a race
     * condition in the painting loop
     */
    if (abs(prev_ut - ut) > 1) shouldPaintSync = true;

    while(prev_ut < ut) {
        drawColumn(++prev_ut + kNumVisibleTilesX);
    }
    while(prev_ut > ut) {
        drawColumn(--prev_ut);
    }

    {
        Vec2 vec((position - kEndCapPadding), kIconYOffset);
        canvas.BG0_setPanning(vec);
    }
}

bool Menu::itemVisibleAtCol(uint8_t item, int column) {
    ASSERT(item >= 0 && item < numItems);
    if (column < 0) return false;

    return itemAtCol(column) == item;
}

uint8_t Menu::itemAtCol(int column) {
    if (column < 0) return numItems;

    if (column % kItemTileWidth < kIconTileWidth) {
        return column < 0 ? numItems : column / kItemTileWidth;
    }
    return numItems;
}

int Menu::computeCurrentTile() {
    /* these are necessary if the icon widths are an odd number of tiles,
     * because the position is no longer aligned with the tile domain.
     */
    const int kPositionAlignment = kEndCapPadding % kPixelsPerTile == 0 ? 0 : kPixelsPerTile - kEndCapPadding % kPixelsPerTile;
    const int kTilePadding = kEndCapPadding / kPixelsPerTile;

    int ui = position - kPositionAlignment;
    int ut = (ui < 0 ? ui - kPixelsPerTile : ui) / kPixelsPerTile; // special case because int rounds up when < 0
    ut -= kTilePadding;

    return ut;
}
