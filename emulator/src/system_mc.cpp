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
#include "flash_volume.h"
#include "usbprotocol.h"
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

    FlashDevice::init();
    FlashBlock::init();
    USBProtocolHandler::init();
    Crc32::init();

    if (!instance->sys->opt_headless) {
        AudioOutDevice::init(&AudioMixer::instance);
        AudioOutDevice::start();
    } else {
        AudioOutDevice::initStub();
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
    // Install a launcher

    const char *launcher = sys->opt_launcherFilename.empty() ? NULL : sys->opt_launcherFilename.c_str();
    if (!sys->flash.installLauncher(launcher))
        return;

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
}

void SystemMC::threadFn(void *param)
{
    if (setjmp(instance->mThreadExitJmp)) {
        // Any actual cleanup on exit would go here...
        return;
    }

    // Start the master at some point shortly after the cubes come up
    instance->ticks = instance->sys->time.clocks + MCTiming::STARTUP_DELAY;
    instance->radioPacketDeadline = instance->ticks + MCTiming::TICKS_PER_PACKET;

    HomeButton::init();
    Volume::init();
    Radio::init();

    instance->autoInstall();

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

void Radio::init()
{
    // Nothing to do in simulation
}

void Radio::begin()
{
    // Nothing to do in simulation - hardware requires a delay between init
    // and the beginning of transmissions.
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

void SystemMC::doRadioPacket()
{
    // Prepare buffers
    struct {
        PacketTransmission ptx;
        PacketBuffer prx;
        Cube::Radio::Packet packet;
        Cube::Radio::Packet reply;
        bool ack;
    } buf;
    memset(&buf, 0, sizeof buf);
    buf.ptx.packet.bytes = buf.packet.payload;
    buf.prx.bytes = buf.reply.payload;

    // MC firmware produces a packet
    RadioManager::produce(buf.ptx);
    ASSERT(buf.ptx.dest != NULL);
    buf.packet.len = buf.ptx.packet.len;

    // Simulates (hardware * software) retries
    static const uint32_t MAX_RETRIES = 150;

    for (unsigned retry = 0; retry < MAX_RETRIES; ++retry) {

        /*
         * Deliver it to the proper cube.
         *
         * Interaction with the cube simulation must take place
         * between beginEvent() and endEvent() only.
         *
         * Note that this causes us to sync the Cube thread's clock with
         * radioPacketDeadline, which slightly lags our 'ticks' counter,
         * which slightly lags the internal SvmCpu cycle count.
         *
         * The timestamp we give to endEvent() is the farthest we allow
         * the Cube thread to run asynchronously before waiting for us again.
         */

        sys->getCubeSync().beginEventAt(radioPacketDeadline, mThreadRunning);
        Cube::Hardware *cube = getCubeForAddress(buf.ptx.dest);
        buf.ack = cube && cube->spi.radio.handlePacket(buf.packet, buf.reply);
        radioPacketDeadline += MCTiming::TICKS_PER_PACKET;
        sys->getCubeSync().endEvent(radioPacketDeadline);

        // Log this transaction
        if (sys->opt_radioTrace) {
            LOG(("RADIO: %6dms %02d/%02x%02x%02x%02x%02x -- TX[%2d] ",
                int(SysTime::ticks() / SysTime::msTicks(1)),
                buf.ptx.dest->channel,
                buf.ptx.dest->id[4],
                buf.ptx.dest->id[3],
                buf.ptx.dest->id[2],
                buf.ptx.dest->id[1],
                buf.ptx.dest->id[0],
                buf.packet.len));

            // Nybbles in little-endian order. With the exception
            // of flash-escaped bytes, the TX packets are always nybble streams.

            for (unsigned i = 0; i < sizeof buf.packet.payload; i++) {
                if (i < buf.packet.len) {
                    uint8_t b = buf.packet.payload[i];
                    LOG(("%x%x", b & 0xf, b >> 4));
                } else {
                    LOG(("  "));
                }
            }

            // ACK data segmented into struct pieces

            if (buf.ack) {
                LOG((" -- Cube %d: ACK[%2d] ", cube->id(), buf.reply.len));
                for (unsigned i = 0; i < buf.reply.len; i++) {
                    switch (i) {
                        case RF_ACK_LEN_FRAME:
                        case RF_ACK_LEN_ACCEL:
                        case RF_ACK_LEN_NEIGHBOR:
                        case RF_ACK_LEN_FLASH_FIFO:
                        case RF_ACK_LEN_BATTERY_V:
                        case RF_ACK_LEN_HWID:
                            LOG(("-"));
                    }
                    LOG(("%02x", buf.reply.payload[i]));
                }
                LOG(("\n"));
            } else {
                LOG((" -- TIMEOUT, retry #%d\n", retry));
            }
        }

        // Send the response
        if (buf.ack) {
            if (buf.reply.len) {
                buf.prx.len = buf.reply.len;
                RadioManager::ackWithPacket(buf.prx);
            } else {
                RadioManager::ackEmpty();
            }
            return;
        }
    }
    
    // Out of retries
    RadioManager::timeout();
}

Cube::Hardware *SystemMC::getCubeForSlot(CubeSlot *slot)
{
    return instance->getCubeForAddress(slot->getRadioAddress());
}

Cube::Hardware *SystemMC::getCubeForAddress(const RadioAddress *addr)
{
    uint64_t packed = addr->pack();

    for (unsigned i = 0; i < sys->opt_numCubes; i++) {
        Cube::Hardware &cube = sys->cubes[i];
        if (cube.spi.radio.getPackedRXAddr() == packed)
            return &cube;
    }

    return NULL;
}

void SystemMC::checkQuiescentVRAM(CubeSlot *slot)
{
    /*
     * For debugging only.
     *
     * This function can be called at points where we know there are no
     * packets in-flight and no data that still needs to be encoded from
     * the cube's vbuf. At these quiescent points, we should be able to
     * ASSERT that our SYSVideoBuffer matches the cube's actual VRAM.
     */

    ASSERT(slot);
    _SYSVideoBuffer *vbuf = slot->getVBuf();
    if (!vbuf)
        return;

    Cube::Hardware *hw = getCubeForSlot(slot);
    if (!hw)
        return;

    unsigned errors = 0;

    ASSERT(vbuf->cm16 == 0);
    for (unsigned i = 0; i < arraysize(vbuf->cm1); i++) {
        if (vbuf->cm1[i] != 0) {
            LOG(("VRAM[%d]: Changes still present in cm1[%d], 0x%08x\n",
                slot->id(), i, vbuf->cm1[i]));
            errors++;
        }
    }

    const uint8_t *hwMem = hw->cpu.mExtData;
    const uint8_t *bufMem = vbuf->vram.bytes;

    for (unsigned i = 0; i < _SYS_VRAM_BYTES; i++) {
        if (hwMem[i] != bufMem[i]) {
            LOG(("VRAM[%d]: Mismatch at 0x%03x, hw=%02x buf=%02x\n",
                slot->id(), i, hwMem[i], bufMem[i]));
            errors++;
        }
    }

    if (errors) {
        LOG(("VRAM[%d]: %d total errors\n", slot->id(), errors));
        ASSERT(0);
    }

    DEBUG_LOG(("VRAM[%d]: okay!\n", slot->id()));
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
