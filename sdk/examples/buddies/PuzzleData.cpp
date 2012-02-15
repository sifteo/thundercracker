////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 - TRACER. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PuzzleData.h"
#include "Puzzle.h"
#include "Piece.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////
// These arrays represent different configurations of the puzzle space. The first dimension of
// the array is the number of cubes in play, the second dimension are the sides of the cube.
// 
// So in other words, each side of each cube is assigned a instance of a "Piece" class. A Piece
// represents a part of the face (holding which buddy it originated from, which face part it is,
// which attributes it has, and whether or not the piece must be in place to solve the puzzle). 
//
// For example: Piece(1, 2, ATTRIBUTE_HIDDEN, true) would be the mouth of Buddy 1 (the blue cat)
// but it is a tricky hidden piece and it is necessary to be in the right spot to solve the puzzle.
// Attributes default to ATTRIBUTE_NORMAL and "must solve" defaults to true.
//
// Attributes are: ATTRIBUTE_NORMAL, ATTRIBUTE_FIXED, and ATTRIBUTE_HIDDEN.
//
// Define each possible configuation puzzles can have up here. We will assign them to the 
// start and end state of puzzle below.
////////////////////////////////////////////////////////////////////////////////////////////////////

// The default configuration: all buddies with their parts in the proper place
Piece kDefaultState[kNumCubes][NUM_SIDES] =
{
    {
        Piece(0, 0, Piece::ATTRIBUTE_NORMAL, true),
        Piece(0, 1, Piece::ATTRIBUTE_NORMAL, true),
        Piece(0, 2, Piece::ATTRIBUTE_NORMAL, true),
        Piece(0, 3, Piece::ATTRIBUTE_NORMAL, true),
    },
    {
        Piece(1, 0, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 1, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 2, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 3, Piece::ATTRIBUTE_NORMAL, true),
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Piece(BuddyId, PartId, Attribute, MustSolve)
// - BuddyId = [0...kNumCubes)
// - PartId = [0...NUM_SIDES)
// - Attribute = [Piece::ATTRIBUTE_NORMAL, Piece::ATTRIBUTE_FIXED, Piece::ATTRIBUTE_HIDDEN]
// - MustSolve = true/false
////////////////////////////////////////////////////////////////////////////////////////////////////

Piece kStartStateAttributeTest[kNumCubes][NUM_SIDES] =
{
    // Buddy 0
    {
        Piece(0, 0, Piece::ATTRIBUTE_NORMAL, true), // Top
        Piece(0, 1, Piece::ATTRIBUTE_NORMAL, true), // Left
        Piece(0, 2, Piece::ATTRIBUTE_HIDDEN, true), // Bottom
        Piece(0, 3, Piece::ATTRIBUTE_NORMAL, true), // Right
    },
    // Buddy 1
    {   
        Piece(1, 0, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 1, Piece::ATTRIBUTE_FIXED,  true),
        Piece(1, 2, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 3, Piece::ATTRIBUTE_FIXED,  true),
    },
};

Piece kAuthoredEndStateMouths[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(0, 0, Piece::ATTRIBUTE_NORMAL, false),
        Piece(0, 1, Piece::ATTRIBUTE_NORMAL, false),
        Piece(1, 2, Piece::ATTRIBUTE_NORMAL, true),
        Piece(0, 3, Piece::ATTRIBUTE_NORMAL, false),
    },
    {
        Piece(1, 0, Piece::ATTRIBUTE_NORMAL, false),
        Piece(1, 1, Piece::ATTRIBUTE_NORMAL, false),
        Piece(0, 2, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 3, Piece::ATTRIBUTE_NORMAL, false),
    },
};

Piece kAuthoredEndStateHair[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(1, 0, Piece::ATTRIBUTE_NORMAL, true),
        Piece(0, 1, Piece::ATTRIBUTE_NORMAL, false),
        Piece(0, 2, Piece::ATTRIBUTE_NORMAL, false),
        Piece(0, 3, Piece::ATTRIBUTE_NORMAL, false),
    },
    {
        Piece(0, 0, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 1, Piece::ATTRIBUTE_NORMAL, false),
        Piece(1, 2, Piece::ATTRIBUTE_NORMAL, false),
        Piece(1, 3, Piece::ATTRIBUTE_NORMAL, false),
    },
};

Piece kAuthoredEndStateEyes[kMaxBuddies][NUM_SIDES] =
{
    {
        Piece(0, 0, Piece::ATTRIBUTE_NORMAL, false),
        Piece(1, 1, Piece::ATTRIBUTE_NORMAL, true),
        Piece(0, 2, Piece::ATTRIBUTE_NORMAL, false),
        Piece(1, 3, Piece::ATTRIBUTE_NORMAL, true),
    },
    {
        Piece(1, 0, Piece::ATTRIBUTE_NORMAL, false),
        Piece(0, 1, Piece::ATTRIBUTE_NORMAL, true),
        Piece(1, 2, Piece::ATTRIBUTE_NORMAL, false),
        Piece(0, 3, Piece::ATTRIBUTE_NORMAL, true),
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// This is the default state (all buddies with their original parts in the right position).
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle kPuzzleDefault = Puzzle(kNumCubes, "DEFAULT", kDefaultState, kDefaultState);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Here are all of our puzzles. Each puzzle has the number of cubes it uses, a string for
// instrunctions, the start state, and the end state. Just use kNumCubes for the number of buddies
// for now. Be aware that the instrunctions currently only support two lines of 16 characters each.
//
// Puzzle(NUMBER_OF_CUBES, INSTRUCTIONS, START_STATE, END_STATE);
//
// Just add new puzzles to this array to throw them into play. Puzzle mode starts at the first
// puzzle and rotate through them all after each solve. It currently just loops back to puzzle 0 if
// all are solved.
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle kPuzzles[] =
{
    Puzzle(kNumCubes, "SWAP MOUTHS", kDefaultState, kAuthoredEndStateMouths),
    Puzzle(kNumCubes, "SWAP HAIR",   kDefaultState, kAuthoredEndStateHair),
    Puzzle(kNumCubes, "SWAP EYES",   kDefaultState, kAuthoredEndStateEyes),
    Puzzle(kNumCubes, "ATTRIBUTE\nTEST", kStartStateAttributeTest, kAuthoredEndStateMouths),
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
