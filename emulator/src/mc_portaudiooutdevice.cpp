/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "mc_portaudiooutdevice.h"
#include "audiomixer.h"
#include "tasks.h"
#include "macros.h"


PortAudioOutDevice::PortAudioOutDevice() :
    outStream(0) {}

int PortAudioOutDevice::portAudioCallback(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags, void *userData)
{
    // Prevent unused variable warnings.
    (void) timeInfo;
    (void) inputBuffer;

    /*
     * Copy from the source AudioBuffer to our simulation-only supplemental
     * buffer which covers up the jitter in our virtual clock.
     */

    PortAudioOutDevice *self = static_cast<PortAudioOutDevice*>(userData);
    SimBuffer_t &ring = self->simBuffer;
    ring.pull(self->buf);

    /*
     * Copy from the intermediate buffer to PortAudio's
     * variable-sized output buffer. If we run out of data
     * early, we must pad with silence. This could indicate
     * an actual underrun condition, or it could just be the
     * end of all playing samples.
     */

    int16_t *outBuf = (int16_t*)outputBuffer;
    unsigned avail = ring.readAvailable();
    unsigned count = MIN(framesPerBuffer, avail);

    framesPerBuffer -= count;
    while (count--)
        *outBuf++ = ring.dequeue();

    while (framesPerBuffer--)
        *outBuf++ = 0;

    return paContinue;
}

void PortAudioOutDevice::init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer)
{
    this->mixer = mixer;

    PaDeviceIndex outDeviceIndex = Pa_GetDefaultOutputDevice();
    if (outDeviceIndex == paNoDevice) {
        LOG(("AUDIO: Couldn't get output device :(\n"));
        return;
    }

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(outDeviceIndex);
    if (devInfo == NULL) {
        LOG(("AUDIO: Couldn't get device info :(\n"));
        return;
    }

    static const struct PaStreamParameters outParams = {
        outDeviceIndex,                     // device
        1,                                  // channelCount - mono only
        paInt16,                            // sampleFormat
        devInfo->defaultLowOutputLatency,   // suggestedLatency
        NULL                                // hostApiSpecificStreamInfo
    };

    buf.init();

    int rate = 0;
    switch (samplerate) {
    case AudioOutDevice::kHz8000: rate = 8000; break;
    case AudioOutDevice::kHz16000: rate = 16000; break;
    case AudioOutDevice::kHz32000: rate = 32000; break;
    }

    PaError err = Pa_OpenStream(&outStream,
                                NULL,                           //inputParameters,
                                &outParams,
                                rate,
                                paFramesPerBufferUnspecified,
                                paClipOff | paDitherOff,        // turn off additional processing
                                PortAudioOutDevice::portAudioCallback,
                                this);
    // TODO - we may want to specify the frames per buffer above...right now,
    // the host system is high enough latency that we need to provide larger
    // audio buffers when running on the host as opposed to the master HW.
    // Perhaps this would be a way to mitigate that.

    if (err != paNoError) {
        LOG(("AUDIO: Couldn't open stream :(\n"));
        return;
    }

    Tasks::setPending(Tasks::AudioPull, &buf);
}

void PortAudioOutDevice::start()
{
    PaError err = Pa_StartStream(outStream);
    if (err != paNoError) {
        LOG(("AUDIO: Couldn't start stream :(\n"));
        return;
    }
}

void PortAudioOutDevice::stop()
{
    PaError err = Pa_StopStream(outStream);
    if (err != paNoError) {
        LOG(("AUDIO: Couldn't stop stream :(\n"));
        return;
    }
}
