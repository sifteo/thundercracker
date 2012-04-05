#ifndef WORDGAME_H
#define WORDGAME_H

#include <sifteo.h>
#include "GameStateMachine.h"
#include "menu.h"

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
    WordGame(Cube cubes[], Menu &m);
    void update(float dt);

    bool needsPaintSync() const { return mNeedsPaintSync; }
    void setNeedsPaintSync() { mNeedsPaintSync = true; }
    void paintSync() { System::paintSync(); mNeedsPaintSync = false; }

    static WordGame* instance() { ASSERT(sInstance); return sInstance; }
    static Menu* getMenu() {ASSERT(sMenu); return sMenu; }
    static void hideSprites(VidMode_BG0_SPR_BG1 &vid);
    static void onEvent(unsigned eventID, const EventData& data);
    static bool playAudio(_SYSAudioModule &mod,
                          AudioChannelIndex channel = AudioChannelIndex_Music,
                          _SYSAudioLoopType loopMode = LoopOnce,
                          AudioPriority priority = AudioPriority_Normal);

    static void getRandomCubePermutation(unsigned char *indexArray);

    static Math::Random random;

private:
    bool _playAudio(_SYSAudioModule &mod, AudioChannelIndex channel,
                    _SYSAudioLoopType loopMode,
                    AudioPriority priority);
    void _onEvent(unsigned eventID, const EventData& data);

    GameStateMachine mGameStateMachine;
    AudioChannel mAudioChannels[NumAudioChannelIndexes];
    bool mNeedsPaintSync;
    AudioPriority mLastAudioPriority[NumAudioChannelIndexes];
    static WordGame* sInstance;
    static Menu* sMenu;
};

#endif // WORDGAME_H
