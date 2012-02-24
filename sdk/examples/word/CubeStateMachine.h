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
#include "Anim.h"

using namespace Sifteo;

const unsigned MAX_LETTERS_PER_CUBE = 3;
const unsigned MAX_LETTERS_PER_WORD = MAX_LETTERS_PER_CUBE * NUM_CUBES;// TODO longer words post CES: _SYS_NUM_CUBE_SLOTS * GameStateMachine::getCurrentMaxLettersPerCube();

class CubeStateMachine : public StateMachine
{
public:
    CubeStateMachine() :
        StateMachine(0), mNumLetters(1), mIdleTime(0.f), mAnimTime(0.f), mAnimIndex(AnimIndex_Tile2Idle),
        mBG0Panning(0.f), mBG0TargetPanning(0.f), mBG0PanningLocked(true), mLettersStart(0), mLettersStartTarget(0),
        mCube(0) {}

    void setCube(Cube& cube);
    Cube& getCube();

    virtual unsigned getNumStates() const;
    virtual State& getState(unsigned index);

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual void update(float dt);
    void sendEventToRow(unsigned eventID, const EventData& data);

    void resetStateTime() { mStateTime = 0.0f; }

    bool getLetters(char *buffer, bool forPaint=false);
    void queueAnim(AnimIndex anim);/*
                   VidMode_BG0_SPR_BG1 &vid,
                    BG1Helper *bg1 = 0,
                    const AnimParams *params = 0);*/
    void queueDefaultAnimForState();
            /*VidMode_BG0_SPR_BG1 &vid,
                                  BG1Helper *bg1 = 0,
                                  const AnimParams *params = 0);*/

    void updateAnim(VidMode_BG0_SPR_BG1 &vid,
                     BG1Helper *bg1 = 0,
                     const AnimParams *params = 0);

    bool canBeginWord();
    bool beginsWord(bool& isOld, char* wordBuffer, bool& isBonus);
    unsigned findRowLength();
    bool isConnectedToCubeOnSide(Cube::ID cubeIDStart, Cube::Side side=SIDE_LEFT);
    bool hasNoNeighbors() const;
    float getIdleTime() const { return mIdleTime; }
    bool canNeighbor() const { return (int)mBG0Panning == (int)mBG0TargetPanning; }
    int getPanning() const { return (int)mBG0Panning; }

private:
    void setPanning(VidMode_BG0_SPR_BG1& vid, float panning);
    AnimIndex getAnimForCurrentState() const;

    // shared state data
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    Vec2 mTilePositions[MAX_LETTERS_PER_CUBE];
    unsigned mNumLetters;
    float mIdleTime;
    float mAnimTime;
    AnimIndex mAnimIndex;

    float mBG0Panning;
    float mBG0TargetPanning;
    bool mBG0PanningLocked;
    unsigned mLettersStart;
    unsigned mLettersStartTarget;

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
