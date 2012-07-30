/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _UI_COORDINATOR_H
#define _UI_COORDINATOR_H

#include <sifteo/abi.h>


/**
 * This object handles high-level cube management and state management
 * for the built-in UI screens. It tracks connected and disconnected
 * cubes, manages stippling and restoring cubes, and handles video buffer
 * and paint state.
 *
 * This is used by all of the built-in UIs except for UIPanic, which
 * takes a much more minimal approach in order to run in any execution
 * context.
 */

class UICoordinator {
public:
    UICoordinator(uint32_t excludedTasks=0);

    // Poll for new cubes
    _SYSCubeIDVector connectCubes();

    // Bulk operations on all cubes
    void stippleCubes(_SYSCubeIDVector cv);
    void restoreCubes(_SYSCubeIDVector cv);

    // One-cube-at-a-time operations, using 'avb'
    void attachToCube(_SYSCubeID id);
    void paint();
    void finish();
    void detach();

    void idle();

    _SYSAttachedVideoBuffer avb;
    _SYSCubeIDVector uiConnected;
    uint32_t excludedTasks;

private:
    _SYSVideoBuffer *savedVBuf;
};


#endif
