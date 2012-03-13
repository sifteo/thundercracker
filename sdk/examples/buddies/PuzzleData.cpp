////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 - TRACER. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PuzzleData.h"
#include "Puzzle.h"
#include "Piece.h"
#include <sifteo/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////
// These arrays represent different configurations of the puzzle space. The first dimension of
// the array is the number of cubes in play, the second dimension are the sides of the cube.
//  
// So in other words, each side of each cube is assigned a instance of a "Piece" class. A Piece
// represents a part of the face (holding which buddy it originated from, which face part it is,
// whether or not the piece must be in place to solve the puzzle and which attribute it has). 
// 
// For example: Piece(1, 2, true, ATTR_HIDDEN) would be the mouth of Buddy 1 (the blue cat),
// it is necessary to be in the right spot to solve the puzzle, but it is a tricky hidden piece!
// Attributes default to ATTR_NONE.
// 
// Define each possible configuation puzzles can have up here. We will assign them to the 
// start and end state of puzzle below.
////////////////////////////////////////////////////////////////////////////////////////////////////

// The default configuration: all buddies with their parts in the proper place
Piece kDefaultState[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(0, 0, true),
        Piece(0, 1, true),
        Piece(0, 2, true),
        Piece(0, 3, true),
    },
    {
        Piece(1, 0, true),
        Piece(1, 1, true),
        Piece(1, 2, true),
        Piece(1, 3, true),
    },
    {
        Piece(2, 0, true),
        Piece(2, 1, true),
        Piece(2, 2, true),
        Piece(2, 3, true),
    },
    {
        Piece(3, 0, true),
        Piece(3, 1, true),
        Piece(3, 2, true),
        Piece(3, 3, true),
    },
    {
        Piece(4, 0, true),
        Piece(4, 1, true),
        Piece(4, 2, true),
        Piece(4, 3, true),
    },
    {
        Piece(5, 0, true),
        Piece(5, 1, true),
        Piece(5, 2, true),
        Piece(5, 3, true),
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Piece(BuddyId, PartId, MustSolve, Attribute = ATTR_NONE)
// - BuddyId = [0...kNumCubes)
// - PartId = [0...NUM_SIDES)
// - MustSolve = true/false
////////////////////////////////////////////////////////////////////////////////////////////////////

Piece kAuthoredStartStateMouths[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(0, 0, false),
        Piece(0, 1, false),
        Piece(0, 2, false),
        Piece(0, 3, false),
    },
    {
        Piece(1, 0, false),
        Piece(1, 1, false),
        Piece(1, 2, false),
        Piece(1, 3, false),
    },
};

Piece kAuthoredEndStateMouths[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(0, 0, false),
        Piece(0, 1, false),
        Piece(1, 2, true),
        Piece(0, 3, false),
    },
    {
        Piece(1, 0, false),
        Piece(1, 1, false),
        Piece(0, 2, true),
        Piece(1, 3, false),
    },
};

Piece kAuthoredStartStateMixedUp[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(1, 0, false),
        Piece(1, 1, false),
        Piece(1, 2, false),
        Piece(1, 3, false),
    },
    {
        Piece(2, 0, false),
        Piece(2, 1, false),
        Piece(2, 2, false),
        Piece(2, 3, false),
    },
};

Piece kAuthoredEndStateMixedUp[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(1, 0, true),
        Piece(1, 1, true),
        Piece(1, 2, true),
        Piece(1, 3, true),
    },
    {
        Piece(2, 0, true),
        Piece(2, 1, true),
        Piece(2, 2, true),
        Piece(2, 3, true),
    },
};

Piece kAuthoredStartStateHair[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(3, 0, false),
        Piece(3, 1, false),
        Piece(3, 2, false),
        Piece(3, 3, false),
    },
    {
        Piece(4, 0, false),
        Piece(4, 1, false),
        Piece(4, 2, false),
        Piece(4, 3, false),
    },
};

Piece kAuthoredEndStateHair[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(4, 0, true),
        Piece(3, 1, false),
        Piece(3, 2, false),
        Piece(3, 3, false),
    },
    {
        Piece(3, 0, true),
        Piece(4, 1, false),
        Piece(4, 2, false),
        Piece(4, 3, false),
    },
};

Piece kAuthoredStartStateEyes[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(5, 0, false),
        Piece(5, 1, false),
        Piece(5, 2, false),
        Piece(5, 3, true),
    },
    {
        Piece(0, 0, false),
        Piece(0, 1, false),
        Piece(0, 2, false),
        Piece(0, 3, false),
    },
};

Piece kAuthoredEndStateEyes[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(5, 0, false),
        Piece(0, 1, true),
        Piece(5, 2, false),
        Piece(0, 3, true),
    },
    {
        Piece(0, 0, false),
        Piece(5, 1, true),
        Piece(0, 2, false),
        Piece(5, 3, true),
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// This is the default state (all buddies with their original parts in the right position).
////////////////////////////////////////////////////////////////////////////////////////////////////

const char kCutsceneTextStartDefault[][32] =
{
    "Default",
};
const char kCutsceneTextEndDefault[][32] =
{
    "Default",
};

const unsigned int kBuddyIdsDefault[] = { 0, 1, 2, 3, 4, 5 };

const Puzzle kPuzzleDefault =
    Puzzle(
        "Default",
        kCutsceneTextStartDefault, arraysize(kCutsceneTextStartDefault),
        kCutsceneTextEndDefault, arraysize(kCutsceneTextEndDefault),
        "Default",
        kBuddyIdsDefault,
        kNumCubes,
        0,
        kDefaultState,
        kDefaultState);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Here are all of our puzzles. Each puzzle has the number of cubes it uses, a string for
// instrunctions, the start state, and the end state. Be aware that the instructions currently
// only support two lines of 16 characters each.
//
// Puzzle(
//     CHAPTER_TITLE,
//     CUTSCENE_TEXT_START,
//     CUTSCENE_TEXT_END,
//     CLUE,
//     BUDDY_ARRAY,
//     NUMBER_OF_BUDDIES,
//     NUMBER_OF_SHUFFLES,
//     START_STATE,
//     END_STATE);
//
// Just add new puzzles to this array to throw them into play. Puzzle mode starts at the first
// puzzle and rotate through them all after each solve. It currently just loops back to puzzle 0 if
// all are solved.
////////////////////////////////////////////////////////////////////////////////////////////////////

const char kCutsceneTextStart0[][32] =
{
    "[A]Gimme a kiss",
    "[B]Can I use\nyour mouth?",
    "[A]...",
    "[A]OK",
};
const char kCutsceneTextEnd0[][32] =
{
    "[B]Muuuahhh",
    "[A]Hot n'\nheavy!",
};

const char kCutsceneTextStart1[][32] =
{
    "[A]Let's get\nCRAZY!",
};
const char kCutsceneTextEnd1[][32] =
{
    "[B]My head\nhurts.",
};

const char kCutsceneTextStart2[][32] =
{
    "[A]How do I get\ncool hair",
    "[A]like you?",
};
const char kCutsceneTextEnd2[][32] =
{
    "[A]Now I look\nlike Kelly!",
};

const char kCutsceneTextStart3[][32] =
{
    "[A]See the world\nfrom my eyes!",
};
const char kCutsceneTextEnd3[][32] =
{
    "[A]That's much\nbetter.",
};

const unsigned int kBuddiesPuzzle0[] = { 0, 1 };
const unsigned int kBuddiesPuzzle1[] = { 1, 2 };
const unsigned int kBuddiesPuzzle2[] = { 3, 4 };
const unsigned int kBuddiesPuzzle3[] = { 5, 0 };
const unsigned int kBuddiesPuzzle4[] = { 0, 1 };

const Puzzle kPuzzles[] =
{
    Puzzle(
        "Big Mouth",
        kCutsceneTextStart0, arraysize(kCutsceneTextStart0),
        kCutsceneTextEnd0, arraysize(kCutsceneTextEnd0),
        "Swap Mouths",
        kBuddiesPuzzle0, arraysize(kBuddiesPuzzle0),
        0,
        kAuthoredStartStateMouths,
        kAuthoredEndStateMouths),
    Puzzle(
        "All Mixed Up",
        kCutsceneTextStart1, arraysize(kCutsceneTextStart1),
        kCutsceneTextEnd1, arraysize(kCutsceneTextEnd1),
        "Unscramble",
        kBuddiesPuzzle1, arraysize(kBuddiesPuzzle1),
        3,
        kAuthoredStartStateMixedUp,
        kAuthoredEndStateMixedUp),
    Puzzle(
        "Bad Hair Day",
        kCutsceneTextStart2, arraysize(kCutsceneTextStart2),
        kCutsceneTextEnd2, arraysize(kCutsceneTextEnd2),
        "Swap Hair",
        kBuddiesPuzzle2, arraysize(kBuddiesPuzzle2),
        0,
        kAuthoredStartStateHair,
        kAuthoredEndStateHair),
    Puzzle(
        "Private Eyes",
        kCutsceneTextStart3, arraysize(kCutsceneTextStart3),
        kCutsceneTextEnd3, arraysize(kCutsceneTextEnd3),
        "Swap Eyes",
        kBuddiesPuzzle3, arraysize(kBuddiesPuzzle3),
        0,
        kAuthoredStartStateEyes,
        kAuthoredEndStateEyes),
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumPuzzles()
{
    return arraysize(kPuzzles);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle &GetPuzzleDefault()
{
    return kPuzzleDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle &GetPuzzle(unsigned int puzzleIndex)
{
    ASSERT(puzzleIndex < arraysize(kPuzzles));
    return kPuzzles[puzzleIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
