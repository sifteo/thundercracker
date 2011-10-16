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
static const Color helpHintColor(1, 1, 1, 0.5);
static const Color ghostColor(1,1,1,0.3);


FrontendOverlay::FrontendOverlay()
    : messageTimer(0), helpVisible(false) {}

void FrontendOverlay::init(GLRenderer *_renderer, System *_sys)
{
    renderer = _renderer;
    sys = _sys;

    timer.init(&sys->time);

    realTimeMessage[0] = '\0';
    
    for (unsigned i = 0; i < System::MAX_CUBES; i++)
        cubes[i].fps[0] = '\0';
}

void FrontendOverlay::draw()
{    
    // Time stats, updated periodically
    const float statsInterval = 0.5f;
    
    timer.capture();
    if (timer.realSeconds() > statsInterval) {
        float rtPercent = timer.virtualRatio() * 100.0f;

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
            cubes[i].lcd_wr.update(timer, sys->cubes[i].lcd.getWriteCount());
            snprintf(cubes[i].fps, sizeof cubes[i].fps,
                     "#%d - %.1f FPS", i, cubes[i].lcd_wr.getHZ());
        }

        timer.start();
    }

    /*
     * Drawing
     */
    
    moveTo(renderer->getWidth() - margin, margin);
    text(helpHintColor, "Press 'H' for help", 1.0f);
    
    moveTo(margin, margin);
    x += 152;
    text(realTimeColor, realTimeMessage, 1.0f);
    x -= 152;

    if (helpVisible) {
        drawHelp();
    }

    if (messageTimer) {
        text(msgColor, message.c_str());
        messageTimer--;
    }
}       

void FrontendOverlay::drawCube(FrontendCube *fe, unsigned x, unsigned y)
{
    unsigned id = fe->getId();

    moveTo(x, y);

    // Do we need to disambiguate which cube is being debugged?
    if (sys->cubes[id].isDebugging() && sys->opt_numCubes > 1) {
        static const Color debugColor(1,0.5,0.5,1);
        text(debugColor, "Debugging", 0.5);
    }

    text(ghostColor, cubes[id].fps, 0.5);
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

void FrontendOverlay::drawHelp()
{
    static const char *lines[] = {
        "Drag a cube by its center to pull and align it",
        "Drag by an edge or corner to pull and rotate it",
        "While pulling, Right-click or Space to hover, again to rotate",
        "Shift-drag or Right-drag to tilt a cube",
        "Mouse wheel resizes the play surface\n",
        "'S' - Screenshot, 'F' - Fullscreen, 'Z' - Zoom, +/- Adds/removes cubes",
    };
    
    const unsigned numLines = sizeof lines / sizeof lines[0];
    for (unsigned i = 0; i < numLines; i++)
        renderer->overlayText(margin, renderer->getHeight() -
                              margin - (numLines - i) * lineSpacing,
                              helpTextColor.v, lines[i]);
}

    