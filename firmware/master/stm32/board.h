/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef BOARD_H
#define BOARD_H

// available boards to choose from
#define BOARD_TC_MASTER_REV1    1
#define BOARD_TC_MASTER_REV2    2

// default board
#ifndef BOARD
#define BOARD   BOARD_TC_MASTER_REV1
#endif

#include "hardware.h"

#if BOARD == BOARD_TC_MASTER_REV2
#include "board/rev2.h"
#elif BOARD == BOARD_TC_MASTER_REV1
#include "board/rev1.h"
#else
#error BOARD not configured
#endif

#endif // BOARD_H
