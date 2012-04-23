#ifndef WORDGAME_H
#define WORDGAME_H

#include <sifteo.h>
#include "GameStateMachine.h"
#include <sifteo/menu.h>
#include "SavedData.h"

using namespace Sifteo;

union EventData;


//class Menu;

enum AudioChannelIndex
{
    AudioChannelIndex_Music,
    AudioChannelIndex_Neighbor = 1,
    AudioChannelIndex_Score = 1,   // HACK only 2 channels are funcitoning for now
    AudioChannelIndex_Bonus = 1,
    AudioChannelIndex_Teeth = 1,
    AudioChannelIndex_Shake = 1,
    AudioChannelIndex_Time = 1,

    NumAudioChannelIndexes
};

enum AudioPriority
{
    AudioPriority_None,
    AudioPriority_Low,
    AudioPriority_Normal,
    AudioPriority_High,

    NumAudioPriorities
};

class WordGame
{
public:
    WordGame(VideoBuffer vidBuffers[], CubeID pMenuCube, Menu &m);
    void update(float dt);

    bool needsPaintSync() const { return mNeedsPaintSync; }
    void setNeedsPaintSync() { mNeedsPaintSync = true; }
    void paintSync() { System::paint(); System::finish(); mNeedsPaintSync = false; }

    static WordGame* instance() { ASSERT(sInstance); return sInstance; }
    Menu* getMenu() { return mMenu; }
    const CubeID getMenuCube() { return mMenuCube; }
    const SavedData& getSavedData() const { return mSavedData; }
    static void hideSprites(VideoBuffer &vid);
    static void onEvent(unsigned eventID, const EventData& data);
    static bool playAudio(const AssetAudio &mod,
                          AudioChannelIndex channel = AudioChannelIndex_Music,
                          AudioChannel::LoopMode loopMode = AudioChannel::ONCE,
                          AudioPriority priority = AudioPriority_Normal);

    static void getRandomCubePermutation(unsigned char *indexArray);

    static Random random;

private:
    bool _playAudio(const AssetAudio &mod,
                    AudioChannelIndex channel,
                    AudioChannel::LoopMode loopMode,
                    AudioPriority priority);
    void _onEvent(unsigned eventID, const EventData& data);

    GameStateMachine mGameStateMachine;
    AudioChannel mAudioChannels[NumAudioChannelIndexes];
    bool mNeedsPaintSync;
    AudioPriority mLastAudioPriority[NumAudioChannelIndexes];
    Menu* mMenu;
    CubeID mMenuCube;
    SavedData mSavedData;

    static WordGame* sInstance;
};

#endif // WORDGAME_H
