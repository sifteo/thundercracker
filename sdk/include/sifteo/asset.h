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

/**
 * @defgroup assets Assets
 *
 * @brief Objects for loading, using, and inspecting visual and audio assets
 *
 * Audio and visual artwork packaged with a game are referred to here as _assets_.
 * This module details the ways that Sifteo Cubes store and load assets. Many of the
 * objects here are instantiated by `stir`. See the @ref asset_workflow for details.
 *
 * @{
 */

#ifndef NOT_USERSPACE
#   include <sifteo/asset/group.h>
#   include <sifteo/asset/loader.h>
#endif

#include <sifteo/asset/audio.h>
#include <sifteo/asset/image.h>

/**
 * @} endgroup menu
 */
