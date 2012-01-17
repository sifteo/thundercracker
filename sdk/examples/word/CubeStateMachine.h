#ifndef CUBESTATEMACHINE_H
#define CUBESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "TitleCubeState.h"
#include "TitleExitCubeState.h"
#include "ScoredCubeState_NotWord.h"
#include "ScoredCubeState_NewWord.h"
#include "ScoredCubeState_OldWord.h"
#include "ScoredCubeState_StartOfRound.h"
#include "ScoredCubeState_EndOfRound.h"
#include "ScoredCubeState_Shuffle.h"
#include "config.h"

using namespace Sifteo;

const unsigned MAX_LETTERS_PER_CUBE = 2;
const unsigned MAX_LETTERS_PER_WORD = MAX_LETTERS_PER_CUBE * NUM_CUBES;// TODO longer words post CES: _SYS_NUM_CUBE_SLOTS * MAX_LETTERS_PER_CUBE;

class CubeStateMachine : public StateMachine
{
public:
    CubeStateMachine() :
        StateMachine(0), mNumLetters(1), mIdleTime(0.f),
        mBG0Panning(0.f), mBG0TargetPanning(0.f), mCube(0) {}

    void setCube(Cube& cube);
    Cube& getCube();

    virtual unsigned getNumStates() const;
    virtual State& getState(unsigned index);

    virtual void onEvent(unsigned eventID, const EventData& data);    
    virtual void update(float dt);
    void sendEventToRow(unsigned eventID, const EventData& data);

    void resetStateTime() { mStateTime = 0.0f; }

    bool getLetters(char *buffer, bool forPaint=false);
    bool canBeginWord();
    bool beginsWord(bool& isOld, char* wordBuffer);
    unsigned findRowLength();
    bool isConnectedToCubeOnSide(Cube::ID cubeIDStart, Cube::Side side=SIDE_LEFT);
    bool hasNoNeighbors() const;
    float getIdleTime() const { return mIdleTime; }
    int getPanning() const { return (int) mBG0Panning; }
    bool canNeighbor() const { return (int)mBG0Panning == (int)mBG0TargetPanning; }

private:
    // shared state data
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    unsigned mNumLetters;
    float mIdleTime;

    float mBG0Panning;
    float mBG0TargetPanning;

    Cube* mCube;
    TitleCubeState mTitleState;
    TitleExitCubeState mTitleExitState;
    ScoredCubeState_NotWord mNotWordScoredState;
    ScoredCubeState_NewWord mNewWordScoredState;
    ScoredCubeState_OldWord mOldWordScoredState;
    ScoredCubeState_StartOfRound mStartOfRoundScoredState;
    ScoredCubeState_EndOfRound mEndOfRoundScoredState;
    ScoredCubeState_Shuffle mShuffleScoredState;
};

#endif // CUBESTATEMACHINE_H
