/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK
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
    _SYS_BASE_BT_CONNECT,
    _SYS_BASE_BT_DISCONNECT,
    _SYS_CUBE_ACCELCHANGE,
    _SYS_BASE_TRACKER,
    _SYS_CUBE_BATTERY,
    _SYS_CUBE_REFRESH,
    _SYS_BASE_GAME_MENU,
    _SYS_BASE_VOLUME_DELETE,
    _SYS_BASE_VOLUME_COMMIT,
    _SYS_BASE_BT_READ_AVAILABLE,
    _SYS_BASE_BT_WRITE_AVAILABLE,
    _SYS_BASE_USB_CONNECT,
    _SYS_BASE_USB_DISCONNECT,
    _SYS_BASE_USB_READ_AVAILABLE,
    _SYS_BASE_USB_WRITE_AVAILABLE,

    _SYS_NUM_VECTORS,   // Must be last
} _SYSVectorID;


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
