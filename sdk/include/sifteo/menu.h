/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * These headers define the system Menu object.
 * To use the Menu, you must include sifteo/menu.h.
 * It is not part of the default set of headers in sifteo.h.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

/**
 * @defgroup menu Menu
 *
 * @brief Common tilt-flow menu API
 */

#include <sifteo/menu/types.h>
#include <sifteo/menu/util.h>
#include <sifteo/menu/public.h>
#include <sifteo/menu/states.h>
#include <sifteo/menu/events.h>

/**
 * @} endgroup menu
 */
