#ifndef WORDGAME_H
#define WORDGAME_H

#include <sifteo.h>
#include "GameStateMachine.h"
using namespace Sifteo;

union EventData;

enum AudioChannelIndex
{
    AudioChannelIndex_Music,
    AudioChannelIndex_Neighbor,
    AudioChannelIndex_Score,
    AudioChannelIndex_Bonus,
    AudioChannelIndex_NewAnagram,
    AudioChannelIndex_Time,

    Num_AudioChannelIndexs
};

class WordGame
{
public:
    WordGame(Cube cubes[]);
    void update(float dt);
    static void onEvent(unsigned eventID, const EventData& data);
    static bool playAudio(const _SYSAudioModule &mod, AudioChannelIndex channel = AudioChannelIndex_Music, _SYSAudioLoopType loopMode = LoopOnce);
    static unsigned rand(unsigned max);

private:
    bool _playAudio(const _SYSAudioModule &mod, AudioChannelIndex channel = AudioChannelIndex_Music, _SYSAudioLoopType loopMode = LoopOnce);
    void _onEvent(unsigned eventID, const EventData& data);

    GameStateMachine mGameStateMachine;
    AudioChannel mAudioChannels[Num_AudioChannelIndexs];
    unsigned mRandomSeed;
    static WordGame* sInstance;
};

#endif // WORDGAME_H
