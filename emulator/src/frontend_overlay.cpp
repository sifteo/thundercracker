/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"

static const Color msgColor(1, 1, 0.2);
static const Color helpTextColor(1, 1, 1);
static const Color helpBgColor(0, 0, 0, 0.5);
static const Color helpHintColor(1, 1, 1, 0.5);
static const Color ghostColor(1,1,1,0.3);
static const Color debugColor(1,0.5,0.5,1);
static const Color inspectorBgColor(0, 0, 0, 0.5);
static const Color inspectorTextColor(1, 1, 1);


FrontendOverlay::FrontendOverlay()
    : helpVisible(false), inspectorVisible(false) {}

void FrontendOverlay::init(GLRenderer *_renderer, System *_sys)
{
    renderer = _renderer;
    sys = _sys;

    slowTimer.init(&sys->time);
    fastTimer.init(&sys->time);
    realTimeMessageTimer.init(&sys->time);

    filteredTimeRatio = 1.0f;
    realTimeMessage[0] = '\0';
    
    for (unsigned i = 0; i < System::MAX_CUBES; i++)
        cubes[i].fps[0] = '\0';
}

void FrontendOverlay::draw()
{    
    // Time stats, updated periodically
    const float statsInterval = 0.5f;
        
    slowTimer.capture();
    fastTimer.capture(slowTimer);
    realTimeMessageTimer.capture(slowTimer);
    
    if (slowTimer.realSeconds() > statsInterval) {
        float rtPercent = slowTimer.virtualRatio() * 100.0f;

        // Percent of real-time
        snprintf(realTimeMessage, sizeof realTimeMessage,
                 "%.1f%% real-time", rtPercent);
                 
        // Color-coded time percentage
        if (rtPercent > 90.0f)
            realTimeColor.set(1,1,1);
        else if (rtPercent > 50.0f)
            realTimeColor.set(1, 1, 0.5);
        else
            realTimeColor.set(1, 0.5, 0.5);

        // Calculate cube hardware rates
        
        for (unsigned i = 0; i < sys->opt_numCubes; i++) {
            // FPS (LCD writes per second)
            cubes[i].lcd_wr.update(slowTimer, sys->cubes[i].lcd.getWriteCount());
            snprintf(cubes[i].fps, sizeof cubes[i].fps,
                     "#%d - %.1f FPS", i, cubes[i].lcd_wr.getHZ());
        }

        slowTimer.start();
    }

    /*
     * Drawing
     */
    
    moveTo(renderer->getWidth() - margin, margin);
    text(helpHintColor, "Press 'H' for help", 1.0f);

    if (helpVisible) {
        drawHelp();
    }
    
    moveTo(margin, margin);
    drawRealTimeInfo();
    
    if (messageTimer) {
        text(msgColor, message.c_str());
        messageTimer--;
    }
    
    fastTimer.start();
}       

void FrontendOverlay::drawCubeStatus(FrontendCube *fe, int x, int y)
{
    unsigned id = fe->getId();

    moveTo(x, y);

    // Do we need to disambiguate which cube is being debugged?
    if (sys->cubes[id].isDebugging() && sys->opt_numCubes > 1)
        text(debugColor, "Debugging", 0.5);

    text(ghostColor, cubes[id].fps, 0.5);
}

void FrontendOverlay::drawCubeInspector(FrontendCube *fe,
    int left, int top, int width, int height)
{
    if (!inspectorVisible)
        return;

    unsigned id = fe->getId();
    const int margin = 2;
    static const char *headings[] = {
        "Asset flash:",
        0,
    };

    // Force the box to be wide enough for all our text
    for (unsigned i = 0; headings[i]; i++)
        width = std::max(width, renderer->measureText(headings[i]) + margin*2);

    // Background rectangle
    renderer->overlayRect(left, top, width, height, inspectorBgColor.v);
    moveTo(left + margin, top + margin);

    // Flash memory state
    {
        text(inspectorTextColor, headings[0]);

        // Eliminate unnecessary texture uploads; see if the flash has changed
        uint32_t flashModifyCount = sys->cubes[id].flash.getModifyCount();
        bool hasChanged = flashModifyCount != cubes[id].flashModifyCount;
        cubes[id].flashModifyCount = flashModifyCount;

        // Use all remaining space
        int flashWidth = width - margin * 2;
        int flashHeight = height - margin - (y - top);

        renderer->overlayCubeFlash(id, x, y, flashWidth, flashHeight,
            sys->cubes[id].flashStorage.data.ext, hasChanged);
    }
}

void FrontendOverlay::postMessage(std::string _message)
{
    messageTimer = 120;
    message = _message;
}

void FrontendOverlay::text(const Color &c, const char *msg, float align)
{
    renderer->overlayText(x - renderer->measureText(msg) * align, y, c.v, msg);
    y += lineSpacing;
}

void FrontendOverlay::toggleHelp()
{
    helpVisible ^= true;
}

void FrontendOverlay::toggleInspector()
{
    inspectorVisible ^= true;
}

void FrontendOverlay::drawHelp()
{
    static const char *lines[] = {
        "Drag a cube by its center to pull and align it.",
        "Control-click or middle-click a cube to tap it.",
        "Drag by an edge or corner to pull and rotate it.",
        "While pulling, Right-click or Space to hover, again to rotate.",
        "Shift-drag or Right-drag to tilt a cube.",
        "Mouse wheel resizes the play surface.",
        "'S' - Screenshot, 'F' - Fullscreen, 'T' - Turbo, 'I' - Inspector",
        "'Z' - Zoom, '1' - 1:1 view, '2' - 2x view.",
        "Backspace toggles rotation lock.",
        "+/- Adds/removes cubes.",
        "",
        APP_COPYRIGHT,
    };
    
    const unsigned numLines = sizeof lines / sizeof lines[0];
    const unsigned w = renderer->getWidth();
    const unsigned h = renderer->getHeight();
    const int top = h - margin * 2 - numLines * lineSpacing;

    renderer->overlayRect(0, top, w, h, helpBgColor.v);
    moveTo(margin, top + margin);
    
    for (unsigned i = 0; i < numLines; i++)
        text(helpTextColor, lines[i]);
}

void FrontendOverlay::drawRealTimeInfo()
{
    const unsigned width = 160;
    const unsigned barH = 3;

    // Filter the time ratio a bit. The fastTimer is really jumpy
    float ratio = fastTimer.virtualRatio();
    filteredTimeRatio += 0.05f * (ratio - filteredTimeRatio);
    
    // If we've been close to 100% for a while, hide the indicator
    if (filteredTimeRatio < 0.95f || sys->opt_turbo)
        realTimeMessageTimer.start();
    if (realTimeMessageTimer.realSeconds() < 1.0f) {
    
        // Right-justify the text so it doesn't bounce so much
        x += width;
        text(realTimeColor, realTimeMessage, 1.0f);
        x -= width;

        // Include a tiny bargraph, to show the rate visually
        int barW = width * std::min(1.0f, filteredTimeRatio);
        renderer->overlayRect(x, y, barW, barH, realTimeColor.v);
        y += barH + margin;
    }

    /*
     * Mode Indicators
     */

    if (sys->opt_turbo)
        text(realTimeColor, "Turbo Mode");
    
    if (sys->tracer.isEnabled())
        text(debugColor, "Trace Enabled");
}
