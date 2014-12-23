/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

#ifndef BOARD_H
#define BOARD_H

// available boards to choose from
#define BOARD_NONE              0   // corresponds to _SYS_HW_VERSION_NONE (see abi/types.h)
#define BOARD_TC_MASTER_REV1    1
#define BOARD_TC_MASTER_REV2    2
#define BOARD_TEST_JIG          3
#define BOARD_TC_MASTER_REV3    4

// default board
#ifndef BOARD
#define BOARD   BOARD_TC_MASTER_REV2
#endif

#if BOARD == BOARD_TEST_JIG
#ifndef TEST_JIG_REV
#define TEST_JIG_REV    2
#endif
#endif

#include "hardware.h"

#if BOARD == BOARD_TC_MASTER_REV3
#include "board/rev3.h"
#elif BOARD == BOARD_TC_MASTER_REV2
#include "board/rev2.h"
#elif BOARD == BOARD_TC_MASTER_REV1
#include "board/rev1.h"
#elif BOARD == BOARD_TEST_JIG
#include "board/testjig.h"
#else
#error BOARD not configured
#endif

#endif // BOARD_H
