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

void PortAudioOutDevice::pullFromMixer()
{
    /*
     * Copy from the source AudioBuffer to our simulation-only supplemental
     * buffer which covers up the jitter in our virtual clock.
     *
     * Runs only from Task context. (Our simBuffer requires no more than one writer)
     */

    ASSERT(bufferThreshold > 0);
    simBuffer.pull(AudioMixer::output, bufferThreshold);

    if (simBuffer.readAvailable() < bufferThreshold)
        Tasks::trigger(Tasks::AudioPull);
}

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
    unsigned outputRemaining = framesPerBuffer;

    /*
     * Make sure we're buffering at least twice what the OS asks for
     * at each callback. If not, go back to pre-filling the buffer.
     */
    unsigned minThreshold = MIN(ring.capacity(), framesPerBuffer << 1);
    if (self->bufferThreshold < minThreshold) {
        self->bufferThreshold = minThreshold;
        self->bufferFilling = true;
    }

    // Ask for more audio data
    Tasks::trigger(Tasks::AudioPull);

    if (self->bufferFilling) {
        /*
         * Waiting for buffer to fill
         */

        self->resetWaterMark();

        if (ring.readAvailable() >= self->bufferThreshold)
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

        while (outputRemaining) {
            if (upsampleCounter == 0) {
                if (ring.empty()) {
                    /*
                     * An underrun happened, oh noes. To make sure
                     * we can recover from this, give the ring buffer
                     * time to completely fill before we start pulling
                     * from it again, and increase our adaptive buffer
                     * fill threshold.
                     */
                    self->bufferFilling = true;
                    self->bufferThreshold = MIN(ring.capacity(), self->bufferThreshold << 1);
                    break;
                }
                lastSample = ring.dequeue();
                upsampleCounter = kUpsampleFactor;
            }
            upsampleCounter--;

            *outBuf++ = self->upsampleFilter(lastSample);
            outputRemaining--;
        }

        self->lastSample = lastSample;
        self->upsampleCounter = upsampleCounter;

        /*
         * Can we shrink the buffer? If it's been a while since the
         * buffer has been under a particular fill level, we can try
         * shrinking our buffer by some portion of that level.
         */

        unsigned fillLevel = ring.readAvailable();
        if (fillLevel < self->lowWaterMark) {
            self->lowWaterMark = fillLevel;
            self->lowWaterSamples = 0;
        } else {
            self->lowWaterSamples += framesPerBuffer;
            if (self->lowWaterSamples > NUM_WATERMARK_SAMPLES) {
                int newThreshold = self->bufferThreshold - (self->lowWaterMark / 2);
                newThreshold = MIN(ring.capacity(), MAX(minThreshold, newThreshold));
                self->bufferThreshold = newThreshold;
                self->resetWaterMark();
            }
        }
    }

    // Underrun or mixer is disabled. Nothing to play but silence.
    // (But make sure to filter the silence, to avoid discontinuities)
    while (outputRemaining--)
        *outBuf++ = self->upsampleFilter(0);

    return paContinue;
}

void PortAudioOutDevice::init()
{
    ASSERT(Pa_Initialize() == paNoError);

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

    PaError err = Pa_OpenStream(&outStream,
                                NULL,                           //inputParameters,
                                &outParams,
                                AudioMixer::SAMPLE_HZ * kUpsampleFactor,
                                paFramesPerBufferUnspecified,
                                paClipOff | paDitherOff,        // turn off additional processing
                                PortAudioOutDevice::portAudioCallback,
                                this);

    if (err != paNoError) {
        LOG(("AUDIO: Couldn't open stream :(\n"));
        return;
    }
}

void PortAudioOutDevice::start()
{
    /*
     * Default buffer level. We can adjust it up and down dynamically,
     * but a reasonably large starting buffer size makes it less likely
     * our audio will glitch during startup, when we're busy anyway.
     */
    simBuffer.init();
    bufferFilling = true;
    bufferThreshold = 4096;
    resetWaterMark();

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

    STATIC_ASSERT(kUpsampleFactor == 3);

    const double kInvGain = 0.9 / 3.053118488;

    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = sample * kInvGain;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] = (xv[0] + xv[4]) + 4 * (xv[1] + xv[3]) + 6 * xv[2]
        + ( -0.0870913158 * yv[0]) + ( -0.5963772949 * yv[1])
        + ( -1.5885494090 * yv[2]) + ( -1.9685253995 * yv[3]);

    return clamp(int(yv[4]), -32768, 32767);
}
