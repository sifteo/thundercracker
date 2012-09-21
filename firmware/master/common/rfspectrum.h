#ifndef RFSPECTRUM_H
#define RFSPECTRUM_H

#include "protocol.h"
#include "macros.h"
#include <stdlib.h>

class RFSpectrumModel
{
    /*
     * Attempt to model the noise on the RF spectrum.
     *
     * Right now, we update it based on the number of retries required to send
     * on a given channel, though in the future it's not inconceivable that
     * the base could also go into RX mode and take samples itself.
     *
     * Used to determine when we need to initiate a channel hop, and which
     * channels to allocate for newly connecting cubes.
     */

public:

    /*
     * Bucketize the spectrum - we don't need to track each 1MHz channel
     * independently.
     */
    static const unsigned NUM_BUCKETS = MAX_RF_CHANNEL >> 1;

    /*
     * Buckets with a noise rating beneath this value are considered safe.
     */
    static const unsigned CLEAR_CHANNEL_THRESH = 400;

    // defined in .cpp to avoid circular dep
    static const unsigned MAX_RETRIES;

    RFSpectrumModel() {
        init();
    }

    void init();
    void update(unsigned channel, unsigned retry_count);
    unsigned allocateChannel() const;

    ALWAYS_INLINE bool channelIsClear(unsigned channel) const {
        return buckets[channel >> 1] < CLEAR_CHANNEL_THRESH;
    }

#ifdef SIFTEO_SIMULATOR
    // getter for testing
    unsigned noise(unsigned channel) {
        return buckets[channel >> 1];
    }
#endif

private:

    static const unsigned FIXPT_SHIFT = 16;

    // number of nRF24 channels per WiFi channel
    static const unsigned WIFI_CH_WIDTH = 20;

    static const uint32_t ONE = (1 << FIXPT_SHIFT);

    // Filter gain - lower values for a slower filter
    static const uint32_t GAIN = ONE * 0.7;

    // Constant to apply to distance for scaling factor.
    static const uint32_t K;

    uint16_t buckets[NUM_BUCKETS];

    // fixed point 16.16 scale factor to apply to this channel, based on its
    // distance from the given center channel
    ALWAYS_INLINE static uint32_t scaleFactor(unsigned center, unsigned bucket) {
        unsigned distance = abs(center - bucket);
        return MAX(static_cast<int32_t>(GAIN - distance * K), 0);
    }
};

#endif // RFSPECTRUM_H
