/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    void init();
    void start();
    void stop();
    void pullFromMixer();

private:
    PaStream *outStream;

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
    bool endOfStream;

    // Digital low-pass filter for the above upsampler
    int16_t upsampleFilter(int16_t sample);
    double xv[5], yv[5];

    /*
     * An additional simulation-only buffer, necessary because of the
     * jitter in our virtual clock. We allocate this buffer very large;
     * the amount of it we actually use is determined dynamically.
     *
     * Note that this buffer shouldn't be *too* large. If we're legitimately
     * running too slowly to keep up with the audio output device, this
     * buffer will quickly fill to its capacity and we want the upper-limit
     * on audio outout latency not to be too astronomical.
     */
    typedef RingBuffer<0x10000, int16_t> SimBuffer_t;

    /*
     * How long do we wait (in number of audio samples) before deciding
     * that a new low-water-mark in the buffer has been stable long enough
     * that we can decrease our bufferThreshold? Smaller values will adapt
     * more quickly and be more eager to lower latency, while larger
     * values are more conservative and more likely to avoid underruns.
     */
    static const unsigned NUM_WATERMARK_SAMPLES = 100000;

    /*
     * How many samples do we need before starting a stream? (bufferFilling==true)
     * We don't want to wait for the full bufferThreshold, since that may be nontrivially large.
     */
    static const unsigned FILL_THRESHOLD = 128;

    SimBuffer_t simBuffer;
    uint32_t bufferFilling;     // Must be 32-bit (atomic access)
    uint32_t bufferThreshold;   // Number of samples we'd like to keep buffered

    uint32_t lowWaterMark;      // Smallest buffer fill level we've seen
    uint32_t lowWaterSamples;   // Number of times we've seen the buffer >= lowWaterMark

    void resetWaterMark() {
        lowWaterSamples = 0;
        lowWaterMark = -1;
    }

    static int portAudioCallback(const void *inputBuffer, void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData);
};

#endif // PORTAUDIO_OUT_DEVICE_H_
