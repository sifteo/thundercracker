/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SIFTEO_BUDDIES_APP_H_
#define SIFTEO_BUDDIES_APP_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <sifteo/audio.h>
#include "Config.h"
#include "CubeWrapper.h"
#include "GameState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class Puzzle;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// App holds all the state necessary to run CubeBuddies. System events are forward directly
/// to this class. A single instance should live in main.
///////////////////////////////////////////////////////////////////////////////////////////////////

class App
{
public:
    App();
    
    void Init(); /// Called on program initialization
    void Reset(); /// Called after assets are loaded, or when game is reset
    void Update(float dt);
    void Draw();
    
    /// CubeWrapper Accessors
    const CubeWrapper &GetCubeWrapper(Sifteo::Cube::ID cubeId) const;
    CubeWrapper &GetCubeWrapper(Sifteo::Cube::ID cubeId);
    
    /// Event Notifications
    void OnNeighborAdd(
        Sifteo::Cube::ID cubeId0, Sifteo::Cube::Side cubeSide0,
        Sifteo::Cube::ID cubeId1, Sifteo::Cube::Side cubeSide1);
    void OnTilt(Sifteo::Cube::ID cubeId);
    void OnShake(Sifteo::Cube::ID cubeId);
    
    unsigned int GetNumBestTimes() const;
    float GetBestTime(unsigned int place) const;
    
private:
    void ResetCubesToPuzzle(const Puzzle &puzzle, bool resetBuddies);
    void ResetCubesToShuffleStart();
    void UpdateCubes(float dt);
    
    void PlaySound();
    
    void StartGameState(GameState gameState);
    void UpdateGameState(float dt);
    void DrawGameState();
    void DrawGameStateCube(CubeWrapper &cubeWrapper);
    
    void ShufflePieces(unsigned int numCubes);
    
    void InsertScore();
    
    void SaveData();
    void LoadData();
    
    void UpdateSwap(float dt);
    void OnSwapBegin(unsigned int swapPiece0, unsigned int swapPiece1);
    void OnSwapExchange();
    void OnSwapFinish();
    
    bool IsHinting() const;
    void ChooseHint();
    void StartHint();
    void StopHint(bool reset);
    
    enum TouchState
    {
        TOUCH_STATE_NONE = 0,
        TOUCH_STATE_BEGIN,
        TOUCH_STATE_HOLD,
        TOUCH_STATE_END,
        
        NUM_TOUCH_STATES
    };
    
    bool AnyTouchBegin() const;
    bool AnyTouchHold() const;
    bool AnyTouchEnd() const;
    
    bool HasUnlocked() const;
    
    // Cubes
    CubeWrapper mCubeWrappers[kNumCubes];
    
    // Audio
    Sifteo::AudioChannel mChannel;
    
    // State
    GameState mGameState;
    float mDelayTimer;
    float mOptionsTimer;
    bool mOptionsTouchSync;
    unsigned int mUiIndex;
    bool mUiIndexSync[kNumCubes];
    
    // Input
    TouchState mTouching[kNumCubes];
    
    // Scoring
    float mScoreTimer;
    unsigned int mScoreMoves;
    unsigned int mScorePlace;
    
    // Save Data
    unsigned char mSaveDataStoryProgress;
    float mSaveDataBestTimes[3];
    
    // Swapping
    enum SwapState
    {
        SWAP_STATE_NONE = 0,
        SWAP_STATE_OUT,
        SWAP_STATE_IN,
        
        NUM_SWAP_STATES
    };
    SwapState mSwapState;
    unsigned int mSwapPiece0;
    unsigned int mSwapPiece1;
    float mSwapAnimationSlideTimer;
    float mSwapAnimationRotateTimer;
    
    // Face Complete
    float mFaceCompleteTimers[kNumCubes];
    
    // Scrolling
    Sifteo::Vec2 mBackgroundScroll;
    
    // Hinting
    float mHintTimer;
    int mHintPiece0;
    int mHintPiece1;
    int mHintPieceSkip;
    
    // Clues
    float mClueOnTimer;
    float mClueOffTimers[kNumCubes];
    
    // Free Play Mode
    float mFreePlayShakeThrottleTimer;
    
    // Shuffle Mode
    Piece mShufflePiecesStart[kMaxBuddies][NUM_SIDES];
    unsigned int mShuffleMoveCounter;
    bool mShufflePiecesMoved[kNumCubes * NUM_SIDES];
    
    // Story Mode
    unsigned int mStoryPuzzleIndex;
    unsigned int mStoryCutsceneIndex;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
