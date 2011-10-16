/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"


FrontendOverlay::FrontendOverlay()
    : messageTimer(0) {}

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
    moveTo(margin, margin);
    
    /*
     * Time stats, updated periodically
     */
    
    const float statsInterval = 1.0f;
    
    timer.capture();
    if (timer.realSeconds() > statsInterval) {
        float rtPercent = timer.virtualRatio() * 100.0f;

        // Percent of real-time
        snprintf(realTimeMessage, sizeof realTimeMessage,
                 "%.2f%% real-time", rtPercent);
                 
        // Color-coded time percentage
        if (rtPercent > 90.0f)
            realTimeColor.set(1,1,1);
        else if (rtPercent > 50.0f)
            realTimeColor.set(1, 1, 0.5);
        else
            realTimeColor.set(1, 0.5, 0.5);

        // Calculate cube frame rates
        for (unsigned i = 0; i < sys->opt_numCubes; i++)
            snprintf(cubes[i].fps, sizeof cubes[i].fps,
                     "#%d - %.1f FPS", i,
                     sys->cubes[i].lcd.getWriteCount() / timer.virtualSeconds());

        timer.start();
    }
    
    text(realTimeColor, realTimeMessage);
    
    /*
     * Transient informational messages
     */

    if (messageTimer) {
        static const Color msgColor(1,1,0);
        text(msgColor, message.c_str());
        messageTimer--;
    }
}       

void FrontendOverlay::drawCube(FrontendCube *fe, unsigned x, unsigned y)
{
    static const Color ghostColor(1,1,1,0.3);
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
