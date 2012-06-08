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
    outStream(0), upsampleCounter(0) {}

int PortAudioOutDevice::portAudioCallback(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags, void *userData)
{
    // Prevent unused variable warnings.
    (void) timeInfo;
    (void) inputBuffer;

    int16_t *outBuf = (int16_t*)outputBuffer;
    PortAudioOutDevice *self = static_cast<PortAudioOutDevice*>(userData);
    SimBuffer_t &ring = self->simBuffer;

    /*
     * Copy from the source AudioBuffer to our simulation-only supplemental
     * buffer which covers up the jitter in our virtual clock.
     */
    ring.pull(self->buf);

    if (self->bufferFilling) {
        /*
         * Waiting for buffer to fill
         */
        if (ring.full())
            self->bufferFilling = false;
    } else {
        /*
         * Copy from the intermediate buffer to PortAudio's
         * variable-sized output buffer. If we run out of data
         * early, we must pad with silence. This could indicate
         * an actual underrun condition, or it could just be the
         * end of all playing samples.
         */

        int lastSample = self->lastSample;
        unsigned upsampleCounter = self->upsampleCounter;

        while (framesPerBuffer) {
            if (upsampleCounter == 0) {
                if (ring.empty()) {
                    /*
                     * An underrun happened, oh noes. To make sure
                     * we can recover from this, give the ring buffer
                     * time to completely fill before we start pulling
                     * from it again.
                     */
                    self->bufferFilling = true;
                    break;
                }
                lastSample = ring.dequeue();
                upsampleCounter = kUpsampleFactor;
            }
            upsampleCounter--;

            *outBuf++ = self->upsampleFilter(lastSample);
            framesPerBuffer--;
        }

        self->lastSample = lastSample;
        self->upsampleCounter = upsampleCounter;
    }

    // Underrun or mixer is disabled. Nothing to play but silence.
    // (But make sure to filter the silence, to avoid discontinuities)
    while (framesPerBuffer--)
        *outBuf++ = self->upsampleFilter(0);

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
                                rate * kUpsampleFactor,
                                paFramesPerBufferUnspecified,
                                paClipOff | paDitherOff,        // turn off additional processing
                                PortAudioOutDevice::portAudioCallback,
                                this);

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


int16_t PortAudioOutDevice::upsampleFilter(int16_t sample)
{
    /*
     * This filter is designed for 3x upsampling, i.e. from 16000 to 48000 Hz.
     * It's a fourth order Bessel low-pass filter, with a corner frequency
     * of 1/3 the sample rate.
     *
     * Note that we scale the overall gain by 0.9 to ensure that we
     * won't clip even if the filter overshoots.
     *
     * Designed with http://www-users.cs.york.ac.uk/~fisher/mkfilter
     */

    const double kInvGain = 0.9 / 3.053118488;

    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = sample * kInvGain;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] = (xv[0] + xv[4]) + 4 * (xv[1] + xv[3]) + 6 * xv[2]
        + ( -0.0870913158 * yv[0]) + ( -0.5963772949 * yv[1])
        + ( -1.5885494090 * yv[2]) + ( -1.9685253995 * yv[3]);

    return clamp(int(yv[4]), -32768, 32767);
}
