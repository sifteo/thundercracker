/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "portaudiooutdevice.h"
#include "audiomixer.h"
#include "tasks.h"
#include <sifteo/macros.h>
#include <string.h>

//#define SINE_TEST // uncomment for testing

PortAudioOutDevice::PortAudioOutDevice() :
    outStream(0)
{
}

#ifdef  SINE_TEST

#include "math.h"

#define TABLE_SIZE   (200)
typedef struct {
    float sine[TABLE_SIZE];
    int phase;
    char message[20];
} paTestData;

static paTestData data;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int portAudioTestCallback(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) inputBuffer;
    if (statusFlags != paNoError) {
        //LOG(("statusFlags: %ld\n", statusFlags));
    }

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        *out++ = data->sine[data->phase];  /* left */
        data->phase += 1;
        if (data->phase >= TABLE_SIZE) data->phase -= TABLE_SIZE;
    }

    return paContinue;
}
#endif // SINE_TEST

int PortAudioOutDevice::portAudioCallback(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData)
{
    (void) timeInfo; // Prevent unused variable warnings.
    (void) inputBuffer;

    PortAudioOutDevice *dev = static_cast<PortAudioOutDevice*>(userData);
    if (statusFlags != paNoError) {
        //LOG(("statusFlags: %ld\n", statusFlags));
    }

    AudioBuffer &audiobuf = dev->buf;
    unsigned avail = audiobuf.readAvailable();
    if (avail > 0) {
        uint8_t *outBuf = (uint8_t*)outputBuffer;
        unsigned bytesToWrite = MIN(framesPerBuffer * sizeof(int16_t), avail);
        while (bytesToWrite--)
            *outBuf++ = audiobuf.dequeue();
    }
    else {
        memset(outputBuffer, 0, framesPerBuffer * sizeof(int16_t));
    }
    // TODO: limit how often we try to refill?
    //          on my Win7 machine, waiting until the buffer is 1/2 empty before
    //          refill results in gaps in playback, so just fetching all the time
    //          for now. -- Liam
    Tasks::setPending(Tasks::AudioOutEmpty, &audiobuf);

    return paContinue;
}

#if 0 // unused at the moment
static void portAudioStreamFinished(void* userData)
{
   paTestData *data = (paTestData *) userData;
   LOG(("Stream Completed: %s\n", data->message));
}
#endif

void PortAudioOutDevice::init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer)
{
    this->mixer = mixer;
#ifdef SINE_TEST
    for (int i = 0; i < TABLE_SIZE; i++) {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 4. );
    }
    data.phase = 0;
#endif

    PaDeviceIndex outDeviceIndex = Pa_GetDefaultOutputDevice();
    if (outDeviceIndex == paNoDevice) {
        LOG(("couldn't get output device :(\n"));
        return;
    }

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(outDeviceIndex);
    if (devInfo == NULL) {
        LOG(("couldn't get device info :(\n"));
        return;
    }

    static const struct PaStreamParameters outParams = {
        outDeviceIndex,                     // device
        1,                                  // channelCount - mono only
        paInt16,                            // sampleFormat
        devInfo->defaultLowOutputLatency,   // suggestedLatency
        NULL                                // hostApiSpecificStreamInfo
    };

    buf.init(&this->sysbuf);

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
        LOG(("couldn't open stream :(\n"));
        return;
    }

#if 0 // unused at the moment
    err = Pa_SetStreamFinishedCallback(outStream, &portAudioStreamFinished);
    if (err != paNoError) {
        LOG(("couldn't set streamFinished Callback :(\n"));
        return;
    }
#endif
}

void PortAudioOutDevice::start()
{
    PaError err = Pa_StartStream(outStream);
    if (err != paNoError) {
        LOG(("couldn't start stream :(\n"));
        return;
    }

#if 0
    const int seconds = 3;
    LOG(("Play for %d seconds.\n", seconds));
    Pa_Sleep(seconds * 1000);
    LOG(("CPULoad = %8.6f\n", Pa_GetStreamCpuLoad(outStream)));
#endif
}

void PortAudioOutDevice::stop()
{
    PaError err = Pa_StopStream(outStream);
    if (err != paNoError) {
        LOG(("couldn't stop stream :(\n"));
        return;
    }
}
