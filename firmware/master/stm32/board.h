/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef BOARD_H
#define BOARD_H

// available boards to choose from
#define BOARD_NONE              0   // corresponds to _SYS_HW_VERSION_NONE (see abi/types.h)
#define BOARD_TC_MASTER_REV1    1
#define BOARD_TC_MASTER_REV2    2
#define BOARD_TEST_JIG          3
#define BOARD_TC_MASTER_REV3    4

#if BOARD == BOARD_TEST_JIG
#ifndef TEST_JIG_REV
#define TEST_JIG_REV    2
#endif
#endif

// default board
#ifndef BOARD
#define BOARD   BOARD_TC_MASTER_REV3
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
