/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
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

#ifndef _SYSTEM_MC_H
#define _SYSTEM_MC_H

#include <setjmp.h>
#include <vector>
#include "tinythread.h"
#include "wavefile.h"

class System;
class Radio;
struct SysTime;
struct RadioAddress;
class CubeSlot;

namespace Cube {
    class Hardware;
}


class SystemMC {
 public:
    bool init(System *sys);
    void exit();

    void start();
    void stop();

    static Cube::Hardware *getCubeForSlot(CubeSlot *slot);
    static void checkQuiescentVRAM(CubeSlot *slot);

    /// Queue a game for asynchronous installation. Can be called at any time.
    static bool installGame(const char *name);

    static System *getSystem() {
        return instance->sys;
    }

    // Exit from Siftulator entirely, from within the System simulation thread.
    static void exit(int result);

    /**
     * Cause some time to pass in the MC simulation, and service any
     * asynchronous events that occurred during this elapsed time.
     *
     * Must only be called from the MC thread.
     */
    static void elapseTicks(unsigned n);

    /**
     * Log some audio data. Has no effect unless --waveout was
     * specified on the command line, and the file opened successfully.
     */
    static void logAudioSamples(const int16_t *samples, unsigned count) {
        instance->waveOut.write(samples, count);
    }

    /**
     * How many audio samples should we mix?
     * Used in headless mode, where we have no natural timebase to use.
     * This fabricates an audio clock based on SysTime.
     */
    static unsigned suggestAudioSamplesToMix();

 private:
    static void threadFn(void *);
    void doRadioPacket();
    void autoInstall();
    void pairCube(unsigned cubeID, unsigned pairingID);

    Cube::Hardware *getCubeForAddress(const RadioAddress *addr);

    friend class Radio;
    friend struct SysTime;
    friend class Tasks;

    static SystemMC *instance;
    static std::vector< std::vector<uint8_t> > pendingGameInstalls;
    static tthread::mutex pendingGameInstallLock;

    uint64_t ticks;
    uint64_t radioPacketDeadline;
    uint64_t heartbeatDeadline;

    System *sys;
    WaveWriter waveOut;
    
    tthread::thread *mThread;
    bool mThreadRunning;
    jmp_buf mThreadExitJmp;
};

#endif
