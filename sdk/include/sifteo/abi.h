/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Definition of the Application Binary Interface for Sifteo games.
 *
 * Whereas the rest of the SDK is effectively a layer of malleable
 * syntactic sugar, this defines the rigid boundary between a game and
 * its execution environment. Everything in this file posesses a
 * binary compatibility guarantee.
 */

#ifndef _SIFTEO_ABI_H
#define _SIFTEO_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Data types which are valid across the user/system boundary.
 */

#define NUM_CUBE_IDS   32
typedef uint8_t CubeID;
typedef uint32_t CubeIDVector;

struct AssetGroup;


/**
 * Event vectors. These can be changed at runtime in order to handle
 * events within the game binary. All vectors are NULL (no-op) by
 * default.
 */

struct EventVectors {
    void (*frame)();			/// Perform game logic or drawing updates for one frame
    void (*cubeFound)(CubeID cid);
    void (*cubeLost)(CubeID cid);
    void (*loadAssetsDone)(CubeID cid);
};

extern struct EventVectors SYS_vectors;


/**
 * Entry point to the game binary.
 *
 * If it returns, that is equivalent to entering an infinite
 * SYS_yield() loop. Typically a game uses siftmain() to set up
 * event handlers in SYS_vectors, then it returns.
 */

void siftmain(void);


/**
 * Low-level system call interface.
 */
    
void SYS_exit(void);				/// Explicitly exit the game, returning to the main menu
void SYS_yield(void);				/// Temporarily cede control to the firmware
void SYS_paint(void);				/// Enqueue a new rendering frame
void SYS_setEnabledCubes(CubeIDVector cv);	/// Which cubes will be trying to connect?

void SYS_loadAssets(CubeID cid, struct AssetGroup *group);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
