/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_SYSCALL_H
#define _SIFTEO_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

/// Maximum number of slots for per-cube state in the master firmware.
#define NUM_CUBE_IDS   32


/**
 * Entry point to the game binary.
 */

void siftmain(void);


/**
 * Low-level system call interface.
 */

void SYS_yield(void);
void SYS_draw(void);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
