/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _UI_FAULT_H
#define _UI_FAULT_H

#include "ui_coordinator.h"

/**
 * UIFault is the simple fault message displayed by FaultLogger.
 */

class UIFault {
public:
    UIFault(UICoordinator &uic, unsigned reference)
        : uic(uic), reference(reference) {}

    void init();
    void mainLoop();

private:
    UICoordinator &uic;
    unsigned reference;
};


#endif
