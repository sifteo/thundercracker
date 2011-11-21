
#include "audiooutdevice.h"
#include "portaudiooutdevice.h"
#include <sifteo/macros.h>

/*
 * Host side implementation of AudioOutDevice - just forward calls to the
 * appropriate hw specific device. This sucks at the moment, since
 * we're paying for function call overhead, according to the map file - even at O2.
 *
 * There's probably a better way to organize this, but this at least
 * allows us to keep the same audiooutdevice.h between the cube and the
 * simulator.
 */

PortAudioOutDevice portaudio;

void AudioOutDevice::init(SampleRate samplerate, AudioMixer *mixer)
{
    ASSERT(Pa_Initialize() == paNoError);
    portaudio.init(samplerate, mixer);
}

void AudioOutDevice::start()
{
    portaudio.start();
}

void AudioOutDevice::stop()
{
    portaudio.stop();
}

bool AudioOutDevice::isBusy()
{
    return false;
}

int AudioOutDevice::sampleRate()
{
    return 0;
}

void AudioOutDevice::setSampleRate(SampleRate samplerate)
{
    (void)samplerate;
}

void AudioOutDevice::suspend()
{

}

void AudioOutDevice::resume()
{

}
