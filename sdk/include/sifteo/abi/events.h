/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_EVENTS_H
#define _SIFTEO_ABI_EVENTS_H

#include <sifteo/abi/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Event vectors. These can be changed at runtime in order to handle
 * events within the game binary, via _SYS_setVector / _SYS_getVector.
 */

typedef void (*_SYSCubeEvent)(void *context, _SYSCubeID cid);
typedef void (*_SYSNeighborEvent)(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1);

typedef enum {
    _SYS_NEIGHBOR_ADD = 0,
    _SYS_NEIGHBOR_REMOVE,
    _SYS_CUBE_CONNECT,
    _SYS_CUBE_DISCONNECT,
    _SYS_CUBE_ASSETDONE,
    _SYS_CUBE_TOUCH,
    _SYS_VECTOR_RESERVED_0,
    _SYS_VECTOR_RESERVED_1,
    _SYS_CUBE_ACCELCHANGE,
    _SYS_BASE_TRACKER,
    _SYS_CUBE_BATTERY,
    _SYS_CUBE_REFRESH,
    _SYS_BASE_GAME_MENU,
    _SYS_BASE_VOLUME_DELETE,
    _SYS_BASE_VOLUME_COMMIT,

    _SYS_NUM_VECTORS,   // Must be last
} _SYSVectorID;


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
