#include "rfspectrum.h"
#include "string.h"
#include "radio.h"

const unsigned RFSpectrumModel::MAX_RETRIES =
        PacketTransmission::DEFAULT_HARDWARE_RETRIES *
        PacketTransmission::DEFAULT_SOFTWARE_RETRIES;

const uint32_t RFSpectrumModel::K =
        RFSpectrumModel::ONE * (MAX_RETRIES / WIFI_CH_WIDTH);

void RFSpectrumModel::init()
{
    memset(buckets, 0, sizeof buckets);
}

void RFSpectrumModel::update(unsigned channel, unsigned retry_count)
{
    unsigned bucket = channel >> 1;

    ASSERT(retry_count <= MAX_RETRIES);
    ASSERT(bucket < NUM_BUCKETS);

    for (unsigned i = 0; i < NUM_BUCKETS; ++i) {

        uint32_t sf = scaleFactor(bucket, i);
        uint16_t &bucketVal = buckets[i];

        // fixed point 16.16 linear interpolation
        int64_t result64 = static_cast<int64_t>((bucketVal * (ONE - sf)) + (retry_count * sf));
        bucketVal = result64 >> FIXPT_SHIFT;
    }
}

unsigned RFSpectrumModel::allocateChannel() const
{
    /*
     * Find the channel least likely to be noisy at the moment.
     */

    return 0;
}
