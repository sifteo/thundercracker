/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBE_H
#define _SIFTEO_CUBE_H

namespace Sifteo {


/**
 * Represents one Sifteo cube. This object may have a lifetime that is
 * distinct from that of our actual connection to a physical
 * cube. Applications need to expect Cube instances to disconnect or
 * reappear at any time. A disconnected Cube instance will ignore any
 * data we direct toward it.
 */

class Cube {

};


};  // namespace Sifteo

#endif
