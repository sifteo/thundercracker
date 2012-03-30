#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "TitleGameState.h"
#include "ScoredGameState.h"
#include "ScoredGameState_StartOfRound.h"
#include "ScoredGameState_EndOfRound.h"
#include "ScoredGameState_Shuffle.h"
#include "StoryGameState_CityProgression.h"
#include "CubeStateMachine.h"
#include "Utility.h"
#include "LevelProgressData.h"
#include "Anim.h"

using namespace Sifteo;

enum GameStateIndex
{
    // TODO rename, remove old, states
    GameStateIndex_Title,
    GameStateIndex_PlayScored,
    GameStateIndex_StartOfRoundScored,
    GameStateIndex_EndOfRoundScored,
    GameStateIndex_ShuffleScored,
    GameStateIndex_StoryCityProgression,

    GameStateIndex_NumStates
};

const unsigned char MAX_HINTS = 3;

class GameStateMachine : public StateMachine
{
public:
    GameStateMachine(Cube cubes[]);

    virtual void update(float dt);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

    const LevelProgressData& getLevelProgressData() const { return mLevelProgressData; }

    static CubeStateMachine* findCSMFromID(Cube::ID cubeID);

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
    bool isMetaLetterIndexUnlocked(unsigned char i) const { return (mMetaLetterUnlockedMask & (1 << i)) != 0; }
    void setNumHints(unsigned char i) { mNumHints = i; }
    void initNewMeta();

protected:
    virtual State& getState(unsigned index);
    virtual void setState(unsigned newStateIndex, State& oldState);
    virtual unsigned getNumStates() const { return GameStateIndex_NumStates; }


private:
    TitleGameState mTitleState;
    ScoredGameState mScoredState;
    ScoredGameState_StartOfRound mScoredStartOfRoundState;
    ScoredGameState_EndOfRound mScoredEndOfRoundState;
    ScoredGameState_Shuffle mScoredShuffleState;
    StoryGameState_CityProgression mStoryCityProgressionState;
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


    static GameStateMachine* sInstance;
};

#endif // GAMESTATEMACHINE_H
