
#include "rfspectrum.h"
#include "macros.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static bool allElementsGT(unsigned *elements, unsigned count, unsigned thresh)
{
    unsigned gtCount = 0;
    
    for (unsigned i = 0; i < count; ++i) {
        if (elements[i] > thresh)
            gtCount++;
    }
    return gtCount == count;
}

static int megaFakeNormalDist(int mean, unsigned maxdistance)
{
    /*
     * A *terrible* approximation of a normal distribution.
     * Would be nice to replace this with something closer to
     * to python's random.gauss as used in
     * firmware/master/tools/rfspectrum-model.py
     */

    int distance = rand() % maxdistance;
    if (rand() & 0x1)
        distance = -distance;

    return MAX(mean + distance, 0);
}

static bool channelIsPeak(unsigned ch) {
    // because we bucketize into 2-channel wide buckets,
    // helpful to make sure no adjacent peak channels are
    // in different buckets.
    return ch >= 4 && ch <= 7;
}

static void matchHiddenSpectrum()
{
    /*
     * Create an arbitrary but realistic snapshot of a spectrum.
     * We feed samples into the model based on this snapshot,
     * and ensure that it converges.
     */

    uint16_t hiddenSpectrum[MAX_RF_CHANNEL + 1];
    for (unsigned i = 0; i < arraysize(hiddenSpectrum); ++i) {
        if (channelIsPeak(i))
            hiddenSpectrum[i] = 900;
        else
            hiddenSpectrum[i] = 100;
    }
    
    srand(time(NULL));
    RFSpectrumModel model;
    
    unsigned peaks[4];
    bzero(peaks, sizeof peaks);
    
    for (;;) {
        
        // sample 5 retry counts for a random channel, based on hiddenSpectrum
        unsigned ch = rand() % MAX_RF_CHANNEL;
        for (unsigned i = 0; i < 5; ++i) {
            unsigned retries = megaFakeNormalDist(hiddenSpectrum[ch], 50);
            model.update(ch, retries);
        }
        
        // ensure we've sampled a sufficient number of times from
        // channels our peaks are on
        if (channelIsPeak(ch)) {
            peaks[ch - 4]++;
            
            if (allElementsGT(peaks, arraysize(peaks), 50))
                break;
        }
    }
    
    // verify our peaks are peaks, and others aren't
    for (unsigned i = 0; i < RFSpectrumModel::NUM_BUCKETS; ++i) {
        if (channelIsPeak(i)) {
            ASSERT(model.energry(i) >= 850 && model.energry(i) <= 950);
        } else {
            ASSERT(model.energry(i) >= 50 && model.energry(i) <= 150);
        }
    }
}

int main()
{
    matchHiddenSpectrum();
    
    LOG(("rfspectrum: Success.\n"));
    return 0;
}
