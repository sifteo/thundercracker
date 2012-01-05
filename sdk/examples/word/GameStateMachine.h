#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "TitleGameState.h"
#include "ScoredGameState.h"
#include "ScoredGameState_StartOfRound.h"
#include "ScoredGameState_EndOfRound.h"
#include "ScoredGameState_Shuffle.h"
#include "CubeStateMachine.h"

using namespace Sifteo;

enum GameStateIndex
{
    GameStateIndex_Title,
    GameStateIndex_PlayScored,
    GameStateIndex_StartOfRoundScored,
    GameStateIndex_EndOfRoundScored,
    GameStateIndex_ShuffleScored,

    GameStateIndex_NumStates
};

const float ANAGRAM_COOLDOWN = 2.0f; // TODO reduce when tilt bug is gone
#ifdef DEBUG
const float ROUND_TIME = 7.0f;
#else
const float ROUND_TIME = 120.0f;
#endif
const float ROUND_BONUS_TIME = 10.0f;


class GameStateMachine : public StateMachine
{
public:
    GameStateMachine(Cube cubes[]);

    virtual void update(float dt);
    virtual void onEvent(unsigned eventID, const EventData& data);
    static void sOnEvent(unsigned eventID, const EventData& data);
    static unsigned GetNumCubes() { return MAX_CUBES; }// TODO
    static CubeStateMachine* findCSMFromID(Cube::ID cubeID);

    static float getAnagramCooldown() { return sInstance->mAnagramCooldown; }
    static unsigned getSecondsLeft() { return (unsigned) sInstance->mTimeLeft; }
    static unsigned getScore() { return (unsigned) sInstance->mScore; }
    static float getTime() { return sInstance->StateMachine::getTime(); }
    static unsigned char getNewWordLength() { return sInstance->mNewWordLength; }

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
    CubeStateMachine mCubeStateMachines[MAX_CUBES];
    float mAnagramCooldown;
    float mTimeLeft;
    unsigned mScore;
    unsigned char mNewWordLength;

    static GameStateMachine* sInstance;
};

#endif // GAMESTATEMACHINE_H
