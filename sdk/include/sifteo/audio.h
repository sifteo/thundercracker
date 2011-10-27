/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>

namespace Sifteo {

/*
 * A single module of audio - an encoded sample, a tracker style synth table, etc
 */
class AudioModule {
public:
    uint32_t size;
    const uint8_t *data;
};

// TODO - expose AudioMixer here?

}; // namespace Sifteo

#endif // AUDIO_H_
