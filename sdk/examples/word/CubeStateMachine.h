#ifndef CUBESTATEMACHINE_H
#define CUBESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "ScoredCubeState_NotWord.h"
#include "ScoredCubeState_NewWord.h"
#include "ScoredCubeState_OldWord.h"

using namespace Sifteo;

const unsigned MAX_LETTERS_PER_CUBE = 1;

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
    bool beginsWord(bool& isOld);
    bool isInWord();

private:
    // shared state data
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    unsigned mNumLetters;

    Cube* mCube;
    ScoredCubeState_NotWord mNotWordScoredState;
    ScoredCubeState_NewWord mNewWordScoredState;
    ScoredCubeState_OldWord mOldWordScoredState;
};

#endif // CUBESTATEMACHINE_H
