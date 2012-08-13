/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ASSETLOADER_H
#define _ASSETLOADER_H

#include <sifteo/abi.h>
#include "macros.h"
#include "systime.h"
#include "flash_syslfs.h"

struct PacketBuffer;
struct AssetGroupInfo;


/**
 * The AssetLoader is a global object which coordinates the installation of
 * assets onto cubes. Our system-side AssetLoader is always available, but
 * it lies dormant when no corresponding _SYSAssetLoader is available,
 * as it requires this userspace-managed memory to operate.
 *
 * An AssetLoader can keep track of one AssetConfiguration per cube, also
 * in userspace-managed memory. This configuration acts as a set of
 * instructions for what to load on each cube.
 *
 * We communicate with the Cube's radio ISR via FIFO buffers in the
 * userspace _SYSAssetLoader. We orchestrate the loading process and
 * access flash memory from a Task which fills the FIFO with commands
 * and data.
 */

class AssetLoader
{
public:
    // Userspace-visible operations
    static void start(_SYSAssetLoader *loader, const _SYSAssetConfiguration *cfg,
        unsigned cfgSize, _SYSCubeIDVector cv);
    static void cancel(_SYSCubeIDVector cv);
    static void finish();

    // State management entry points
    static void init();
    static void cubeConnect(_SYSCubeID id);
    static void cubeDisconnect(_SYSCubeID id);

    // Cube radio ISR entry points
    static bool needFlashPacket(_SYSCubeID id);
    static bool needFullACK(_SYSCubeID id);
    static void produceFlashPacket(_SYSCubeID id, PacketBuffer &buf);
    static void ackReset(_SYSCubeID id);
    static void ackData(_SYSCubeID id, unsigned bytes);
    static void queryResponse(_SYSCubeID id, const PacketBuffer &packet);

    /// Tasks callbacks
    static void task();
    static void heartbeat();

    /// Return the current _SYSAssetLoader, if any is attached, or NULL if we're unattached.
    static ALWAYS_INLINE _SYSAssetLoader *getUserLoader() {
        return userLoader;
    }

    /// Which cubes might be busy loading assets right now?
    static ALWAYS_INLINE _SYSCubeIDVector getActiveCubes() {
        return activeCubes;
    }

    /// Which cubes have we validated the cache on?
    static ALWAYS_INLINE _SYSCubeIDVector getCacheCoherentCubes() {
        return cacheCoherentCubes;
    }

    /**
     * In simulation only: We can opt to bypass the actual asset loader, and
     * instead decompress loadstream data directly into cube flash.
     */
    #ifdef SIFTEO_SIMULATOR
    static bool simBypass;
    #endif

private:
    AssetLoader();  // Do not implement

    enum TaskState {
        S_ERROR,                // Internal consistency error; loading will stall.
        S_COMPLETE,             // Done loading, nothing to do.

        S_BEGIN_RESET_STATES,
            S_RESET1,           // Send a state machine reset token, and begin loading preparations
            S_RESET1_WAIT,      // Wait for the cube's state machine to finish resetting
            S_RESET2,           // Second reset packet
            S_RESET2_WAIT,      // Waiting for second reset packet
        S_END_RESET_STATES,
 
        S_CRC_COMMAND,          // Waiting to send a CRC query for one slot (substate = slot bitmap)
        S_CRC_WAIT,             // Waiting for a CRC query response

        S_CONFIG_INIT,          // Begin work on a Configuration step
        S_CONFIG_ADDR,          // Sending load address
        S_CONFIG_DATA,          // Sending AssetGroup data for the current Configuration
        S_CONFIG_FINISH,        // Wait for cube to finsh programming, then finalize SysLFS state
    };

    // State-specific data
    union SubState {
        uint32_t value;

        struct {
            uint32_t remaining;     // Only top 8 bits are actually used
        } crc;

        struct {
            uint32_t index : 8,
                     offset : 24;
        } config;
    };

    // State machine (in assetloader_fsm.cpp)
    static void fsmEnterState(_SYSCubeID id, TaskState s);
    static void fsmTaskState(_SYSCubeID id, TaskState s);

    // Synchronous preparations (Happens while we're waiting for reset)
    static void prepareCubeForLoading(_SYSCubeID id);

    // Simulation-only asset loader bypass hook
    #ifdef SIFTEO_SIMULATOR
    static bool loaderBypass(_SYSCubeID id, AssetGroupInfo &group);
    #endif

    // Copy activeCubes to read-only userspace 'busyCubes' value.
    static ALWAYS_INLINE void updateActiveCubes()
    {
        _SYSAssetLoader *localUserLoader = userLoader;
        if (localUserLoader)
            localUserLoader->busyCubes = activeCubes;
    }

    // When we send data to the cube, update its watchdog timer.
    // Not for use in ISR context.
    static ALWAYS_INLINE void resetDeadline(_SYSCubeID id)
    {
        cubeDeadline[id] = SysTime::ticks() + SysTime::msTicks(350);
    }

    // Pick the next valid query ID, and return it.
    static ALWAYS_INLINE uint8_t nextQueryID(_SYSCubeID id)
    {
        uint8_t q = (cubeLastQuery[id] + 1) | 0x80;
        cubeLastQuery[id] = q;
        return q;
    }

    // Data from userspace
    static _SYSAssetLoader *userLoader;
    static const _SYSAssetConfiguration *userConfig[_SYS_NUM_CUBE_SLOTS];
    static uint8_t userConfigSize[_SYS_NUM_CUBE_SLOTS];

    // Aggregate cube state, set up by high-level entry points
    static _SYSCubeIDVector activeCubes;            // Cubes that are currently loading
    static _SYSCubeIDVector startedCubes;           // Started, and restartable on cubeConnect()
    static _SYSCubeIDVector cacheCoherentCubes;     // We're sure SysLFS state matches actual cube flash mem

    // Task-owned cube state. Read-only from ISR.
    static uint8_t cubeTaskState[_SYS_NUM_CUBE_SLOTS];
    static SubState cubeTaskSubstate[_SYS_NUM_CUBE_SLOTS];
    static SysTime::Ticks cubeDeadline[_SYS_NUM_CUBE_SLOTS];
    static uint8_t cubeLastQuery[_SYS_NUM_CUBE_SLOTS];

    // ISR-owned cube state. Read-only from tasks.
    static uint8_t cubeBufferAvail[_SYS_NUM_CUBE_SLOTS];

    // Atomic shared state
    static _SYSCubeIDVector resetPendingCubes;
    static _SYSCubeIDVector resetAckCubes;
    static _SYSCubeIDVector queryPendingCubes;
    static _SYSCubeIDVector queryErrorCubes;

    DEBUG_ONLY(static SysTime::Ticks groupBeginTimestamp[_SYS_NUM_CUBE_SLOTS];)
};

#endif
