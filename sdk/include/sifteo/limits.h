/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_LIMITS_H
#define _SIFTEO_LIMITS_H

/*
 * Every game can individually define a compile-time limit for the
 * number of supported cubes. It must be less than or equal to the
 * firmware limit (currently 32). Many data structures are statically
 * allocated using this number, so if a game is running low on RAM it
 * can decrease the limit, or if it needs more cubes it can be
 * increased.
 */

#ifndef CUBE_ALLOCATION
#define CUBE_ALLOCATION  12
#endif

#endif
