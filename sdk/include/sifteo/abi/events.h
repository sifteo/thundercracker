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

/*
 * The order of these vector IDs dictates priority, so choose them accordingly.
 * For example, it's important to have accel update last, since it's the spammiest.
 */
typedef enum {
    _SYS_NEIGHBOR_ADD = 0,
    _SYS_NEIGHBOR_REMOVE,
    _SYS_CUBE_FOUND,
    _SYS_CUBE_LOST,
    _SYS_CUBE_ASSETDONE,
    _SYS_CUBE_TOUCH,
    _SYS_CUBE_TILT,
    _SYS_CUBE_SHAKE,
    _SYS_CUBE_ACCELCHANGE,
    _SYS_BASE_TRACKER,

    _SYS_NUM_VECTORS,   // Must be last
} _SYSVectorID;

#define _SYS_NEIGHBOR_EVENTS    ( (0x80000000 >> _SYS_NEIGHBOR_ADD) |\
                                  (0x80000000 >> _SYS_NEIGHBOR_REMOVE) )
#define _SYS_CUBE_EVENTS        ( (0x80000000 >> _SYS_CUBE_FOUND) |\
                                  (0x80000000 >> _SYS_CUBE_LOST) |\
                                  (0x80000000 >> _SYS_CUBE_ASSETDONE) |\
                                  (0x80000000 >> _SYS_CUBE_ACCELCHANGE) |\
                                  (0x80000000 >> _SYS_CUBE_TOUCH) |\
                                  (0x80000000 >> _SYS_CUBE_TILT) |\
                                  (0x80000000 >> _SYS_CUBE_SHAKE) )
#define _SYS_BASE_EVENTS      ( (0x80000000 >> _SYS_BASE_TRACKER) )


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
