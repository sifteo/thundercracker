#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include "ringbuffer.h"
#include "audiooutdevice.h"

/*
 * A type to help ensure that our different flavors of AudioOutDevice
 * all use the same buffer size.
 */
typedef RingBuffer<AudioOutDevice::BufferSize> AudioBuffer;

#endif // AUDIOBUFFER_H
