#include <sifteo.h>
#include "WordGame.h"
#include "assets.gen.h"
#include <cstdlib>

using namespace Sifteo;

WordGame* WordGame::sInstance = 0;
Math::Random WordGame::random;


WordGame::WordGame(Cube cubes[]) : mGameStateMachine(cubes), mNeedsPaintSync(false)
{
    STATIC_ASSERT(NumAudioChannelIndexes == 2);// HACK work around API bug
    sInstance = this;

    for (unsigned i = 0; i < arraysize(mAudioChannels); ++i)
    {
        mAudioChannels[i].init();
        mLastAudioPriority[i] = AudioPriority_None;
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

bool WordGame::playAudio(_SYSAudioModule &mod,
                         AudioChannelIndex channel ,
                         _SYSAudioLoopType loopMode,
                         AudioPriority priority)
{

    switch (channel)
    {
    case AudioChannelIndex_Music:
        if (!MUSIC_ON)
        {
            return false;
        }
        break;

    default:
        if (!SFX_ON)
        {
            return false;
        }
        break;

    }
    ASSERT(sInstance);
    return sInstance->_playAudio(mod, channel, loopMode, priority);
}

bool WordGame::_playAudio(_SYSAudioModule &mod,
                          AudioChannelIndex channel,
                          _SYSAudioLoopType loopMode,
                          AudioPriority priority)
{
    ASSERT((unsigned)channel < arraysize(mAudioChannels));
    bool played = false;
    if (mAudioChannels[channel].isPlaying())
    {
        if (priority >= mLastAudioPriority[channel])
        {
            mAudioChannels[channel].stop();
            played = mAudioChannels[channel].play(mod, loopMode);
        }
    }
    else
    {
        played = mAudioChannels[channel].play(mod, loopMode);
    }

    if (played)
    {
        mLastAudioPriority[channel] = priority;
    }

    return played;
}

void WordGame::hideSprites(VidMode_BG0_SPR_BG1 &vid)
{
    for (int i=0; i < 8; ++i)
    {
        vid.hideSprite(i);
    }
}
