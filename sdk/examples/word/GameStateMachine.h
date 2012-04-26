#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "CubeStateMachine.h"
#include "Utility.h"
#include "LevelProgressData.h"
#include "Anim.h"
#include "CutsceneIndex.h"

using namespace Sifteo;

enum GameStateIndex
{
    // TODO rename, remove old, states
    GameStateIndex_Title,
    GameStateIndex_PlayScored,
    GameStateIndex_StoryStartOfRound,
    GameStateIndex_StartOfRoundScored,
    GameStateIndex_EndOfRoundScored,
    GameStateIndex_ShuffleScored,
    GameStateIndex_StoryCityProgression,
    GameStateIndex_Loading,
    GameStateIndex_MainMenu,
    GameStateIndex_PauseMenu,
    GameStateIndex_Cutscene,
    GameStateIndex_CubeBuddyUnlock,
    GameStateIndex_LoadingFinished,

    GameStateIndex_NumStates
};

const unsigned char MAX_HINTS = 3;
const float TOUCH_ADVANCE_DELAY = .1f; // was .5

class GameStateMachine : public StateMachine
{
public:
    GameStateMachine(VideoBuffer vidBufs[]);

    virtual void update(float dt);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

    const LevelProgressData& getLevelProgressData() const { return mLevelProgressData; }

    static CubeStateMachine* findCSMFromID(CubeID cubeID);

    static GameStateMachine& getInstance() { ASSERT(sInstance); return *sInstance; }
    static float getAnagramCooldown() { return getInstance().mAnagramCooldown; }
    static unsigned getSecondsLeft() { return (unsigned) _ceilf(getInstance().mTimeLeft); }
    static float getSecondsLeftFloat() { return getInstance().mTimeLeft; }
    static unsigned getNumAnagramsLeft() { return getInstance().mNumAnagramsLeft; }
    static unsigned getScore() { return (unsigned) getInstance().mScore; }
    static float getTime() { return getInstance().StateMachine::getTime(); }
    static unsigned char getNewWordLength() { return getInstance().mNewWordLength; }
    static unsigned getNumCubesInAnim(AnimType animT);
    static unsigned getCurrentMaxLettersPerCube();
    static void setCurrentMaxLettersPerCube(unsigned max);
    static unsigned getCurrentMaxLettersPerWord();
    static unsigned sOnEvent(unsigned eventID, const EventData& data);
    static unsigned GetNumCubes() { return NUM_CUBES; }// TODO
    unsigned char getNumHints() const { return mNumHints; }
    bool isMetaLetterIndexUnlocked(unsigned char i) const { return true; }//(mMetaLetterUnlockedMask & (1 << i)) != 0; }
    bool isMetaLetterIndexUnlockedLast(unsigned char i) const { return ((mMetaLetterUnlockedMask ^ mMetaLetterUnlockedMaskOld) & (1 << i)) != 0; }
    void setNumHints(unsigned char i) { mNumHints = i; }
    void initNewMeta();
    CutsceneIndex getCutsceneIndex() const { return mCutsceneIndex; }

protected:
    virtual void setState(unsigned newStateIndex, unsigned oldStateIndex);
    virtual unsigned getNumStates() const { return GameStateIndex_NumStates; }

private:
    static void createNewAnagram();
    static void onAudioEvent(unsigned eventID, const EventData& data);
    CubeStateMachine mCubeStateMachines[NUM_CUBES];
    float mAnagramCooldown;
    float mTimeLeft;
    unsigned mScore;
    unsigned char mNewWordLength;
    unsigned mNumAnagramsLeft;
    unsigned mCurrentMaxLettersPerCube;
    LevelProgressData mLevelProgressData;
    unsigned char mNumHints;
    unsigned mMetaLetterUnlockedMask;
    unsigned mMetaLetterUnlockedMaskOld;
    CubeID mHintCubeIDOnUpdate;
    bool mNeedsNewAnagram;
    CutsceneIndex mCutsceneIndex;

    static GameStateMachine* sInstance;
};

#endif // GAMESTATEMACHINE_H
