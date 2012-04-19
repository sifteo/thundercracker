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
#include "flash.h"
#include "flashlayer.h"
#include "assetmanager.h"
#include "svmloader.h"
#include "svmcpu.h"
#include "svmruntime.h"
#include "mc_gdbserver.h"
#include "cube.h"
#include "protocol.h"

SystemMC *SystemMC::instance;


bool SystemMC::installELF(const char *path)
{
    FILE *elfFile = fopen(path, "rb");
    if (elfFile == NULL) {
        LOG(("Error, couldn't open ELF file '%s' (%s)\n", path, strerror(errno)));
        return false;
    }

    // write the file to external flash
    uint8_t buf[512];
    Flash::chipErase();

    unsigned addr = 0;
    while (!feof(elfFile)) {
        unsigned rxed = fread(buf, 1, sizeof(buf), elfFile);
        if (rxed > 0) {
            Flash::write(addr, buf, rxed);
            addr += rxed;
        }
    }
    fclose(elfFile);
    Flash::flush();

    return true;
}

bool SystemMC::init(System *sys)
{
    this->sys = sys;
    instance = 0;

    Flash::init();
    FlashBlock::init();
    AssetManager::init();

    if (sys->opt_svmTrace)
        SvmCpu::enableTracing();
    if (sys->opt_svmFlashStats)
        FlashBlock::enableStats();
    if (sys->opt_svmStackMonitor)
        SvmRuntime::enableStackMonitoring();

    if (!sys->opt_elfFile.empty() && !installELF(sys->opt_elfFile.c_str()))
        return false;

    return true;
}

void SystemMC::start()
{
    mThreadRunning = true;
    instance = this;
    __asm__ __volatile__ ("" : : : "memory");
    mThread = new tthread::thread(threadFn, 0);
}

void SystemMC::stop()
{
    mThreadRunning = false;
    __asm__ __volatile__ ("" : : : "memory");
    mThread->join();
    delete mThread;
    mThread = 0;
}

void SystemMC::exit()
{
    // Nothing to do yet
}

void SystemMC::threadFn(void *param)
{
    if (setjmp(instance->mThreadExitJmp))
        return;

    // Start the master at some point shortly after the cubes come up
    instance->ticks = instance->sys->time.clocks + STARTUP_DELAY;

    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

    Radio::open();
    GDBServer::start(2345);

    SvmLoader::run(111);
}

SysTime::Ticks SysTime::ticks()
{
    /*
     * Our TICK_HZ divides easily into nanoseconds (62.5 ns at 16 MHz)
     * so we can do this conversion using fixed-point math quite easily.
     *
     * This does it in 64-bit math, with 60.4 fixed-point.
     */

    return ((SystemMC::instance->ticks * hzTicks(SystemMC::TICK_HZ / 16)) >> 4);
}

void Radio::open()
{
    // Nothing to do in simulation
}

void Radio::halt()
{
    SystemMC *smc = SystemMC::instance;

    // Are we trying to stop() the MC thread?
    if (!smc->mThreadRunning)
        longjmp(smc->mThreadExitJmp, 1);

    smc->doRadioPacket();
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

    for (unsigned retry = 0; retry < MAX_RETRIES; ++retry) {

        /*
         * Deliver it to the proper cube.
         *
         * Interaction with the cube simulation must take place
         * between beginPacket() and endPacket() only.
         */
        beginPacket();
        Cube::Hardware *cube = getCubeForAddress(buf.ptx.dest);
        buf.ack = cube && cube->spi.radio.handlePacket(buf.packet, buf.reply);
        endPacket();

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

void SystemMC::beginPacket()
{
    // Advance time, and rally with the cube thread at the proper timestamp.
    // Between beginCubeEvent() and endCubeEvent(), both simulation threads are synchronized.

    ticks += SystemMC::TICKS_PER_PACKET;
    sys->beginCubeEvent(ticks);

    // beginCubeEvent may have already been waiting when we called, and it could
    // be earlier than the deadline we asked for. Now that we're sync'ed, run
    // up until the proper deadline if necessary.

    ASSERT(ticks >= sys->time.clocks);
    while (ticks > sys->time.clocks) {
        sys->endCubeEvent(ticks);
        sys->beginCubeEvent(ticks);
    }
}

void SystemMC::endPacket()
{
    // Let the cube keep running, but no farther than our next transmit opportunity
    sys->endCubeEvent(ticks + SystemMC::TICKS_PER_PACKET);
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

const System *SystemMC::getSystem()
{
    return instance->sys;
}
