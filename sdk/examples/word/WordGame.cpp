#include <sifteo.h>
#include "WordGame.h"
#include "assets.gen.h"

using namespace Sifteo;

WordGame* WordGame::sInstance = 0;
Math::Random WordGame::random;


WordGame::WordGame(Cube cubes[], Cube* pMenuCube, Menu& menu) :
    mGameStateMachine(cubes), mNeedsPaintSync(false)
{
    STATIC_ASSERT(NumAudioChannelIndexes == 2);// HACK work around API bug
    sInstance = this;
    mMenu = &menu;
    mMenuCube = pMenuCube;

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


void WordGame::getRandomCubePermutation(unsigned char *indexArray)
{
    // first scramble the cube to word fragments mapping
    int cubeIndexes[NUM_CUBES];
    for (int i = 0; i < (int)arraysize(cubeIndexes); ++i)
    {
        cubeIndexes[i] = i;
    }

    // assign cube indexes to the puzzle piece indexes array, randomly
    for (int i = 0; i < (int)NUM_CUBES; ++i)
    {
        for (unsigned j = WordGame::random.randrange((unsigned)1, NUM_CUBES);
             true;
             j = ((j + 1) % NUM_CUBES))
        {
            if (cubeIndexes[j] >= 0)
            {
                indexArray[i] = cubeIndexes[j];
                cubeIndexes[j] = -1;
                break;
            }
        }
    }
}
