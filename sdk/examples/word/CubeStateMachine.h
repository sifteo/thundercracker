#ifndef CUBESTATEMACHINE_H
#define CUBESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "TitleCubeState.h"
#include "ScoredCubeState_NotWord.h"
#include "ScoredCubeState_NewWord.h"
#include "ScoredCubeState_OldWord.h"
#include "ScoredCubeState_EndOfRound.h"

using namespace Sifteo;

const unsigned MAX_LETTERS_PER_CUBE = 1;
const unsigned MAX_LETTERS_PER_WORD = 6;// TODO longer words post CES: _SYS_NUM_CUBE_SLOTS * MAX_LETTERS_PER_CUBE;

class CubeStateMachine : public StateMachine
{
public:
    CubeStateMachine() : StateMachine(0), mNumLetters(1), mCube(0) {}

    void setCube(Cube& cube);
    Cube& getCube();

    virtual unsigned getNumStates() const;
    virtual State& getState(unsigned index);

    virtual void onEvent(unsigned eventID, const EventData& data);    
    void sendEventToRow(unsigned eventID, const EventData& data);

    const char* getLetters();
    bool canBeginWord();
    bool beginsWord(bool& isOld, char* wordBuffer);
    unsigned findRowLength();
    bool isConnectedToCubeOnSide(Cube::ID cubeIDStart, Cube::Side side=SIDE_LEFT);
    bool hasNoNeighbors() const;

private:
    // shared state data
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    unsigned mNumLetters;

    Cube* mCube;
    TitleCubeState mTitleState;
    ScoredCubeState_NotWord mNotWordScoredState;
    ScoredCubeState_NewWord mNewWordScoredState;
    ScoredCubeState_OldWord mOldWordScoredState;
    ScoredCubeState_EndOfRound mEndOfRoundScoredState;
};

#endif // CUBESTATEMACHINE_H
