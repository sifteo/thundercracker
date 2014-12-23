/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    void batteryPowerOff();
    void batteryPowerOn();
};

#endif
