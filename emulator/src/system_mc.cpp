/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <setjmp.h>
#include <errno.h>

#include "system.h"
#include "system_mc.h"
#include "macros.h"
#include "radio.h"
#include "systime.h"
#include "audiooutdevice.h"
#include "audiomixer.h"
#include "flash_device.h"
#include "flash_blockcache.h"
#include "flash_stack.h"
#include "flash_volume.h"
#include "flash_syslfs.h"
#include "svmloader.h"
#include "svmcpu.h"
#include "svmruntime.h"
#include "cube.h"
#include "protocol.h"
#include "tasks.h"
#include "mc_timing.h"
#include "lodepng.h"
#include "crc.h"
#include "volume.h"
#include "homebutton.h"
#include "cubeconnector.h"
#include "neighbor_tx.h"
#include "led.h"

SystemMC *SystemMC::instance;
std::vector< std::vector<uint8_t> > SystemMC::pendingGameInstalls;
tthread::mutex SystemMC::pendingGameInstallLock;


bool SystemMC::init(System *sys)
{
    this->sys = sys;
    instance = this;

    if (!sys->opt_waveoutFilename.empty() &&
        !waveOut.open(sys->opt_waveoutFilename.c_str(), AudioMixer::SAMPLE_HZ)) {
        LOG(("AUDIO: Can't open waveout file '%s'\n",
            sys->opt_waveoutFilename.c_str()));
    }

    FlashStack::init();
    Crc32::init();

    if (instance->sys->opt_headless) {
        Tasks::trigger(Tasks::AudioPull);
    } else {
        AudioOutDevice::init();
        AudioOutDevice::start();
    }

    return true;
}

void SystemMC::start()
{
    mThreadRunning = true;
    __asm__ __volatile__ ("" : : : "memory");
    mThread = new tthread::thread(threadFn, 0);
}

void SystemMC::stop()
{
    mThreadRunning = false;
    __asm__ __volatile__ ("" : : : "memory");
    sys->getCubeSync().wake();
    mThread->join();
    delete mThread;
    mThread = 0;
}

void SystemMC::exit()
{
    if (!instance->sys->opt_headless)
        AudioOutDevice::stop();

    waveOut.close();
}

void SystemMC::autoInstall()
{
    // Use stealth flash I/O, for speed
    FlashDevice::setStealthIO(1);

    do {

        // Install a launcher
        const char *launcher = sys->opt_launcherFilename.empty() ? NULL : sys->opt_launcherFilename.c_str();
        if (!sys->flash.installLauncher(launcher))
            break;

        // Install any ELF data that we've previously queued

        tthread::lock_guard<tthread::mutex> guard(pendingGameInstallLock);

        while (!pendingGameInstalls.empty()) {
            std::vector<uint8_t> &data = pendingGameInstalls.back();
            FlashVolumeWriter writer;
            writer.begin(FlashVolume::T_GAME, data.size());
            writer.appendPayload(&data[0], data.size());
            writer.commit();
            pendingGameInstalls.pop_back();
        }

    } while (0);

    FlashDevice::setStealthIO(-1);
}

void SystemMC::pairCube(unsigned cubeID, unsigned pairingID)
{
    // Use stealth flash I/O, for speed
    FlashDevice::setStealthIO(1);

    SysLFS::PairingIDRecord rec;
    ASSERT(cubeID < _SYS_NUM_CUBE_SLOTS);
    ASSERT(pairingID < arraysize(rec.hwid));

    if (!SysLFS::read(SysLFS::kPairingID, rec))
        rec.init();

    uint64_t hwid = sys->cubes[cubeID].getHWID();
    ASSERT(hwid != uint64_t(-1));

    rec.hwid[pairingID] = hwid;
    if (!SysLFS::write(SysLFS::kPairingID, rec)) {
        ASSERT(0);
    }

    FlashDevice::setStealthIO(-1);
}

void SystemMC::threadFn(void *param)
{
    if (setjmp(instance->mThreadExitJmp)) {
        // Any actual cleanup on exit would go here...
        return;
    }

    /*
     * Start the master at some point shortly after the cubes come up,
     * using a no-op deadline event.
     *
     * We can't just use Tasks::waitForInterrupt() here, since the
     * MC is not yet initialized and we don't want to handle radio packets.
     */
     
    // Start the master at some point shortly after the cubes come up
    instance->ticks = instance->sys->time.clocks + MCTiming::STARTUP_DELAY;
    instance->radioPacketDeadline = instance->ticks + MCTiming::TICKS_PER_PACKET;
    instance->heartbeatDeadline = instance->ticks;

    instance->sys->getCubeSync().beginEventAt(instance->ticks, instance->mThreadRunning);
    instance->sys->getCubeSync().endEvent(instance->radioPacketDeadline);

    /*
     * Emulator magic: Automatically install games and pair cubes
     */

    instance->autoInstall();

    for (unsigned i = 0; i < instance->sys->opt_numCubes; i++) {
        /*
         * Create an arbitrary non-identity mapping between cube IDs and
         * pairings, just to help keep us honest in the firmware and
         * catch any places where we get the two confused.
         *
         * (We still keep the cubes in the same order, to reduce confusion...)
         */
        instance->pairCube(i, (i + 8) % _SYS_NUM_CUBE_SLOTS);
    }

    // Subsystem initialization
    HomeButton::init();
    LED::init();
    Volume::init();
    NeighborTX::init();
    CubeConnector::init();
    Radio::init();

    // Start running userspace code!
    SvmLoader::runLauncher();
}

SysTime::Ticks SysTime::ticks()
{
    /*
     * Our TICK_HZ divides easily into nanoseconds (62.5 ns at 16 MHz)
     * so we can do this conversion using fixed-point math quite easily.
     *
     * This does it in 64-bit math, with 60.4 fixed-point.
     */

    return ((SystemMC::instance->ticks * hzTicks(MCTiming::TICK_HZ / 16)) >> 4);
}

void Tasks::waitForInterrupt()
{
    // Elapse time until the next radio packet.
    // Note that we must actually call elapseTicks() here, since it's
    // important to run all async events (including exit) from halt().

    SystemMC *self = SystemMC::instance;
    self->ticks = self->radioPacketDeadline;
    self->elapseTicks(0);
}

Cube::Hardware *SystemMC::getCubeForSlot(CubeSlot *slot)
{
    return instance->getCubeForAddress(slot->getRadioAddress());
}

bool SystemMC::installGame(const char *path)
{
    bool success = true;
    bool restartThread = instance && instance->mThreadRunning;

    if (restartThread)
        instance->stop();

    tthread::lock_guard<tthread::mutex> guard(pendingGameInstallLock);

    pendingGameInstalls.push_back(std::vector<uint8_t>());
    LodePNG::loadFile(pendingGameInstalls.back(), path);
    
    if (pendingGameInstalls.back().empty()) {
        pendingGameInstalls.pop_back();
        success = false;
        LOG(("FLASH: Error, couldn't open ELF file '%s' (%s)\n",
            path, strerror(errno)));
    }

    if (restartThread)
        instance->start();

    return success;
}

void SystemMC::elapseTicks(unsigned n)
{
    SystemMC *self = instance;

    self->ticks += n;

    // Asynchronous exit
    if (!self->mThreadRunning)
        longjmp(self->mThreadExitJmp, 1);

    // Asynchronous radio packets
    while (self->ticks >= self->radioPacketDeadline)
        self->doRadioPacket();

    // Asynchronous task heartbeat
    while (self->ticks >= self->heartbeatDeadline) {
        Tasks::heartbeatISR();
        self->heartbeatDeadline += MCTiming::TICK_HZ / Tasks::HEARTBEAT_HZ;
    }
}

unsigned SystemMC::suggestAudioSamplesToMix()
{
    /*
     * SysTime-based clock for audio logging in --headless mode.
     */

    if (instance->waveOut.isOpen()) {
        unsigned currentSample = SysTime::ticks() / SysTime::hzTicks(AudioMixer::SAMPLE_HZ);
        unsigned prevSamples = instance->waveOut.getSampleCount();
        if (currentSample > prevSamples)
            return currentSample - prevSamples;
    }
    return 0;
}

void SystemMC::exit(int result)
{
    /*
     * Stop the whole simulation, from inside the MC thread.
     *
     * We do need to stop the cube thread first, or it may deadlock
     * in a deadline sync. The exact mechanisms for this can vary
     * depending on platform and luck, but for example we could 
     * deadlock due to exit() calling ~System(), which would
     * destroy the underlying synchronization objects used by
     * the deadline sync.
     *
     * We do *not* want to just ask System to stop everything,
     * since trying to stop the MC simulation from inside the
     * simulation itself would cause a deadlock.
     */

    getSystem()->stopCubesOnly();
    ::exit(result);
}
