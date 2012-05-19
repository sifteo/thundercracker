/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef PORTAUDIO_OUT_DEVICE_H_
#define PORTAUDIO_OUT_DEVICE_H_

#include "portaudio.h"
#include "audiooutdevice.h"
#include "ringbuffer.h"

class AudioMixer;

class PortAudioOutDevice
{
public:
    PortAudioOutDevice();
    void init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer);
    void start();
    void stop();

private:
    PaStream *outStream;
    AudioMixer *mixer;
    RingBuffer<AudioOutDevice::BufferSize> buf;

    static int portAudioCallback(const void *inputBuffer, void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData);
};

#endif // PORTAUDIO_OUT_DEVICE_H_
