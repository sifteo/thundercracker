/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/system.h>
#include "radio.h"

namespace Sifteo {

void System::init()
{
    Radio::open();
}

void System::draw()
{
    Radio::halt();
}

};  // namespace Sifteo
