/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef PORTAUDIO_OUT_DEVICE_H_
#define PORTAUDIO_OUT_DEVICE_H_

#include "portaudio.h"
#include "audiooutdevice.h"

class AudioMixer;

class PortAudioOutDevice
{
public:
    PortAudioOutDevice();
    void init(AudioMixer *mixer);
    void start();
    void stop();

private:
    PaStream *outStream;
    AudioMixer *mixer;
    AudioBuffer buf;

    /*
     * Upsampling amount: Open the host's audio device at this multiple of
     * the requested SampleRate, and output this many copies of every sample.
     *
     * Why bother? Audio drivers are buggy, and 48 kHz is much less likely
     * to tred into untested territory than 16 kHz. Specifically, I added
     * this to avoid a really nasty audio quality bug when Mac OS resamples
     * from 16 to 48 kHz to my external USB DAC.
     *
     * The digital filter parameters and this factor both need to agree
     * on the resampling rate. Currently we assume 3x, for 16 -> 48 kHz.
     */
    static const unsigned kUpsampleFactor = 3;
    unsigned upsampleCounter;
    int lastSample;

    // Digital low-pass filter for the above upsampler
    int16_t upsampleFilter(int16_t sample);
    double xv[5], yv[5];

    /*
     * An additional simulation-only buffer, necessary because of the
     * jitter in our virtual clock. On Windows especially, this is
     * super jittery due to the low Sleep resolution available there.
     * And to make things worse, PA on Windows tends to request
     * much larger audio blocks, making it much easier to underrun.
     *
     * So, use a much larger buffer on Windows.
     */
    #ifdef _WIN32
    typedef RingBuffer<2048, int16_t> SimBuffer_t;
    #else
    typedef RingBuffer<512, int16_t> SimBuffer_t;
    #endif

    SimBuffer_t simBuffer;
    uint32_t bufferFilling;     // Must be 32-bit (atomic access)

    static int portAudioCallback(const void *inputBuffer, void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData);
};

#endif // PORTAUDIO_OUT_DEVICE_H_
