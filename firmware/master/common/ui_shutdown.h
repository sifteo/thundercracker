/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _UI_SHUTDOWN_H
#define _UI_SHUTDOWN_H

#include <sifteo/abi.h>
#include "systime.h"

class UICoordinator;


/**
 * A shutdown countdown screen, giving the user some time to
 * cancel before turning the system off.
 */

class UIShutdown {
public:
    UIShutdown(UICoordinator &uic);

    // Default main loop
    void mainLoop();

    // Entry points for integrating with an external main loop
    void init();
    void animate();

    ALWAYS_INLINE bool isDone() {
        return done;
    }

private:
    UICoordinator &uic;

    SysTime::Ticks digitTimestamp;
    uint8_t digit;
    bool done;

    void drawBackground();
    void drawLogo();
    void drawText(unsigned addr, const char *string);
    void drawDigit(unsigned number);

    void beginDigit(unsigned number);
    void resetTimestamp();

    static float easeInAndOut(float t);
};


#endif
