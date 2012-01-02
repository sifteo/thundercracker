#include <sifteo.h>
#include "WordGame.h"
#include "audio.gen.h"
#include <cstdlib>

using namespace Sifteo;

WordGame* WordGame::sInstance = 0;

WordGame::WordGame(Cube cubes[]) : mGameStateMachine(cubes), mNeedsPaintSync(false)
{
    sInstance = this;

    int64_t nanosec;
    _SYS_ticks_ns(&nanosec);
    mRandomSeed = (unsigned)nanosec;

#ifdef _WIN32
    srand(mRandomSeed); // seed rand()
#endif

    for (unsigned i = 0; i < arraysize(mAudioChannels); ++i)
    {
        mAudioChannels[i].init();

    }
}

void WordGame::update(float dt)
{
    mGameStateMachine.update(dt);
}

void WordGame::onEvent(unsigned eventID, const EventData& data)
{
    if (sInstance)
    {
        sInstance->_onEvent(eventID, data);
    }
}

void WordGame::_onEvent(unsigned eventID, const EventData& data)
{
    mGameStateMachine.onEvent(eventID, data);
}

bool WordGame::playAudio(const _SYSAudioModule &mod, AudioChannelIndex channel , _SYSAudioLoopType loopMode)
{
    ASSERT(sInstance);
    return sInstance->_playAudio(mod, channel, loopMode);
}

bool WordGame::_playAudio(const _SYSAudioModule &mod, AudioChannelIndex channel , _SYSAudioLoopType loopMode)
{
    ASSERT((unsigned)channel < arraysize(mAudioChannels));
    return mAudioChannels[channel].play(mod, loopMode);
}

unsigned WordGame::rand(unsigned max)
{
    ASSERT(sInstance);
#ifdef _WIN32
    return std::rand() % max;
#else
    return rand_r(&sInstance->mRandomSeed) % max;
#endif
}

float WordGame::rand(float min, float max)
{
    return min + ((float)rand((unsigned)(1000.f * (max - min))))/1000.f;
}

void WordGame::hideSprites(VidMode_BG0_SPR_BG1 &vid)
{
    for (int i=0; i < 8; ++i)
    {
        vid.hideSprite(i);
    }
}
