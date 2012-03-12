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
#include "Constants.h"

using namespace Sifteo;

enum CubeAnim
{
    CubeAnim_Main,
    CubeAnim_Hint,
    CubeAnim_Border,

    NumCubeAnims
};

class CubeStateMachine : public StateMachine
{
public:
    CubeStateMachine();
    void setCube(Cube& cube);
    Cube& getCube();

    virtual unsigned getNumStates() const;
    virtual State& getState(unsigned index);

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual void update(float dt);
    void sendEventToRow(unsigned eventID, const EventData& data);

    void resetStateTime() { mStateTime = 0.0f; }

    unsigned getLetters(char *buffer, bool forPaint=false);
    void queueAnim(AnimType anim, CubeAnim cubeAnim=CubeAnim_Main);/*
                   VidMode_BG0_SPR_BG1 &vid,
                    BG1Helper *bg1 = 0,
                    const AnimParams *params = 0);*/
    void queueNextAnim(CubeAnim cubeAnim=CubeAnim_Main);
            /*VidMode_BG0_SPR_BG1 &vid,
                                  BG1Helper *bg1 = 0,
                                  const AnimParams *params = 0);*/

    void updateAnim(VidMode_BG0_SPR_BG1 &vid,
                     BG1Helper *bg1 = 0,
                     AnimParams *params = 0);
    AnimType getAnim() const { return mAnimTypes[CubeAnim_Main]; }

    bool canBeginWord();
    bool beginsWord(bool& isOld, char* wordBuffer, bool& isBonus);
    unsigned findRowLength();
    bool isConnectedToCubeOnSide(Cube::ID cubeIDStart, Cube::Side side=SIDE_LEFT);
    bool hasNoNeighbors() const;
    float getIdleTime() const { return mIdleTime; }
    bool canNeighbor() const { return (int)mBG0Panning == (int)mBG0TargetPanning; }
    int getPanning() const { return (int)mBG0Panning; }

    bool isHintAvailable() const { return mAnimTypes[CubeAnim_Hint] != AnimType_None && mAnimTypes[CubeAnim_Hint] != AnimType_HintDisappear; }
    bool canMakeHintAvailable() const { return !isHintAvailable(); }
    void makeHintAvailable() { queueAnim(AnimType_HintIdle, CubeAnim_Hint); } // TODO hint appear anim
    void removeHint() { queueAnim(AnimType_None, CubeAnim_Hint); }

private:
    void setPanning(VidMode_BG0_SPR_BG1& vid, float panning);
    AnimType getNextAnim(CubeAnim cubeAnim=CubeAnim_Main) const;
    void paint();

    void paintScore(VidMode_BG0_SPR_BG1& vid,
                    ImageIndex teethImageIndex,
                    bool animate=false,
                    bool reverseAnim=false,
                    bool loopAnim=false,
                    bool paintTime=false,
                    float animStartTime=0.f);
    void paintLetters(VidMode_BG0_SPR_BG1 &vid, BG1Helper &bg1, const AssetImage &font, bool paintSprites=false);
    void paintScoreNumbers(BG1Helper &bg1, const Vec2& position, const char* string);

    bool getAnimParams(AnimParams *params);
    void setLettersStart(unsigned s);
    void calcSpriteParams(unsigned i);

    // shared state data
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    char mHintSolution[MAX_LETTERS_PER_CUBE + 1];
    Vec2 mTilePositions[MAX_LETTERS_PER_CUBE];
    unsigned mNumLetters;
    unsigned mPuzzlePieceIndex;
    float mIdleTime;

    AnimType mAnimTypes[NumCubeAnims];
    float mAnimTimes[NumCubeAnims];

    bool mPainting;
    bool mHintRequested;

    float mBG0Panning;
    float mBG0TargetPanning;
    bool mBG0PanningLocked;
    unsigned mLettersStart;
    unsigned mLettersStartOld;

    ImageIndex mImageIndex;
    SpriteParams mSpriteParams;

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
