/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("TinyWave")
    .package("com.sifteo.sdk.wave", "1.0")
    .icon(Icon)
    .cubeRange(1);

const AudioChannel sine1Channel(0);

static int16_t sineWave[256];
static const AssetAudio sineAsset = AssetAudio::fromPCM(sineWave);

void synthInit()
{
    for (int i = 0; i != arraysize(sineWave); i++) {
        float theta = i * float(M_PI * 2 / arraysize(sineWave));
        sineWave[i] = sin(theta) * 0x7fff;
    }
    sine1Channel.play(sineAsset);
}

void synthesize(float hz)
{
    LOG("hz=%f\n", hz);
    //sine1Channel.setVolume(0);
    sine1Channel.setVolume(sine1Channel.MAX_VOLUME);
    sine1Channel.setSpeed(hz * arraysize(sineWave));
}

void main()
{
    synthInit();

    float hz = 1000;
    synthesize(hz);

    while (1) {
        System::paint();
    }
}
