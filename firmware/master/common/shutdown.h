/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SHUTDOWN_H
#define _SHUTDOWN_H

#include "macros.h"

/**
 * This class manages the process of shutting down the master.
 * It runs from the main thread, either within a syscall or a task.
 *
 * After turning off cubes and putting the base into a lower-power
 * state (with no LED or sound), we take some time to do housekeeping
 * tasks before totally powering off.
 *
 * Furthermore, if we're on USB power, we never actually turn off
 * the CPU so we can stay awake servicing USB requests.
 */

class ShutdownManager {
public:
	ShutdownManager(uint32_t excludedTasks);

    void shutdown();

private:
	uint32_t excludedTasks;

    void housekeeping();
    void hardwareShutdown();
};

#endif
