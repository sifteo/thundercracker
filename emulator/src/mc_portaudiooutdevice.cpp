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
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData)
{
    (void) timeInfo; // Prevent unused variable warnings.
    (void) inputBuffer;

    PortAudioOutDevice *dev = static_cast<PortAudioOutDevice*>(userData);
    AudioBuffer &audiobuf = dev->buf;

    unsigned avail = audiobuf.readAvailable();

    if (avail > 0) {
        uint8_t *outBuf = (uint8_t*)outputBuffer;
        unsigned bytesToWrite = MIN(framesPerBuffer * sizeof(int16_t), avail);
        while (bytesToWrite--)
            *outBuf++ = audiobuf.dequeue();
    } else {
        memset(outputBuffer, 0, framesPerBuffer * sizeof(int16_t));
    }

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
