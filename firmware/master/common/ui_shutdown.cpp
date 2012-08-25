/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "ui_shutdown.h"
#include "ui_coordinator.h"
#include "vram.h"
#include "cube.h"
#include "cubeslots.h"
#include "homebutton.h"
#include "led.h"
#include "tasks.h"
#include "svmclock.h"
#include "shutdown.h"

// Shutdown UI Settings
namespace {
    static const unsigned TILE = 8;

    static const unsigned kNumTilesX = 18;
    static const unsigned kNumVisibleTilesX = 16;

    static const unsigned kDigitWidth = 4;
    static const unsigned kDigitHeight = 5;

    static const unsigned kFirstDigit = 5;
    static const float kSlideDuration = SysTime::msTicks(1000);

    static const int kSlideWidth = kNumTilesX * TILE;
    static const int kSlideOrigin = -kSlideWidth + (kDigitWidth * TILE / 2) + 6;
}

UIShutdown::UIShutdown(UICoordinator &uic)
    : uic(uic), digit(kFirstDigit), done(false)
{
    resetTimestamp();
}

void UIShutdown::init()
{
    if (!uic.isAttached())
        return;

    uic.finish();
    uic.letterboxWindow(TILE * uic.assets.shutdownHeight);
    VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_ROM);

    drawBackground();
    drawText(xy(3, uic.assets.shutdownY1), "Shutdown in");
    drawText(xy(1, uic.assets.shutdownY2), "press to cancel");

    uic.setPanX(TILE/2);
    uic.setPanY(0);
    
    uic.paint();
    uic.finish();

    beginDigit(digit);
}

void UIShutdown::resetTimestamp()
{
    digitTimestamp = SysTime::ticks() - SysTime::msTicks(100);
}

void UIShutdown::beginDigit(unsigned number)
{
    digit = number;
    resetTimestamp();
    drawDigit(number);

    uic.letterboxWindow(kDigitHeight * TILE);
}

void UIShutdown::animate()
{
    /*
     * NB: We do want to continue animating even if we aren't attached to a cube.
     *     Shutdown should work the same even when no cubes are attached, using the
     *     LED for visual feedback. In that case, this animation sequence (even though
     *     it isn't visible) still times the shutdown process.
     */

    if (done)
        return;

    CubeSlot &cube = CubeSlots::instances[uic.avb.cube];
    float t = int(SysTime::ticks() - digitTimestamp) / kSlideDuration;

    if (t > 1.0f) {
        if (digit == 1) {
            // Finished the last digit!
            done = true;
            drawLogo();

        } else {
            // Next digit
            beginDigit(digit - 1);

            if (digit == 2) {
                // Beginning the next-to-last digit, blink faster
                LED::set(LEDPatterns::shutdownFast);
            }
        }
        return;
    }

    uic.setPanX(easeInAndOut(t) * kSlideWidth + kSlideOrigin);
}

void UIShutdown::drawBackground()
{
    /*
     * Splats our ShutdownBackground column horizontally across all of BG0
     */

    unsigned addr = 0;
    unsigned height = uic.assets.shutdownHeight;

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < kNumTilesX; ++x) {
            uint16_t tile = uic.assets.shutdownBackground[y];
            VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(tile));
            addr++;
        }
    }
}

void UIShutdown::drawLogo()
{
    uic.finish();
    uic.setPanX(0);
    uic.setPanY(0);
    VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, first_line), 0);
    VRAM::pokeb(uic.avb.vbuf, offsetof(_SYSVideoRAM, num_lines), 128);

    const uint16_t *src = uic.assets.logoWhiteOnBlue;
    unsigned addr = 0;

    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 16; ++x) {
            VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(*src));
            addr++;
            src++;
        }
        addr += 2;
    }
}

void UIShutdown::drawText(unsigned addr, const char *string)
{
    /*
     * Draw a text string to BG0_ROM, using a palette from ShutdownBackground.
     */

    const uint16_t palette = uic.assets.shutdownBackground[1];

    while (*string) {
        uint16_t tile = uint8_t(*string - ' ') ^ palette;
        VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(tile));
        addr++;
        string++;
    }
}

void UIShutdown::drawDigit(unsigned number)
{
    /*
     * Draw one of the 'big digits' at the origin of BG0, using a palette
     * from ShutdownBackground, and filling the rest of the screen with
     * that background tile.
     */

    const uint16_t palette = uic.assets.shutdownBackground[1];
    const uint16_t *frame = uic.assets.bigDigits + number * (kDigitWidth * kDigitHeight);
    unsigned addr = 0;

    for (unsigned y = 0; y < kDigitHeight; ++y) {
        for (unsigned x = 0; x < kNumTilesX; ++x) {
            uint16_t tile = palette;

            if (x < kDigitWidth)
                tile ^= frame[x + y*kDigitWidth];

            VRAM::poke(uic.avb.vbuf, addr, _SYS_TILE77(tile));
            addr++;
        }
    }
}

float UIShutdown::easeInAndOut(float t)
{
    float normalized = t * 2.0f - 1.0f;
    float cubed = normalized * normalized * normalized;
    return cubed * 0.5f + 0.5f;
}

void UIShutdown::mainLoop()
{
    LED::set(LEDPatterns::shutdownSlow, true);
    // only cancel if we see a new press during shutdown process.
    bool haveSeenRelease = HomeButton::isReleased();

    while (1) {

        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            init();

        animate();
        uic.paint();

        if (HomeButton::isReleased())
            haveSeenRelease = true;

        if ((haveSeenRelease && HomeButton::isPressed()) || uic.isTouching()) {
            // Cancel
            uic.restoreCubes(uic.uiConnected);
            LED::set(LEDPatterns::idle);
            Tasks::cancel(Tasks::Pause);
            SvmClock::resume();
            return;
        }

        if (isDone()) {
            uic.finish();
            ShutdownManager s(uic.excludedTasks);
            return s.shutdown();
        }
    }
}
