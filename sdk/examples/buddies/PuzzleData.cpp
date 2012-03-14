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
// InitializePuzzles() is called on init and (surprise surprise) dynamically sets the data for
// all the puzzles. Most functions should be self-explantory, but pay special attention to the
// StartState and EndState ones. These set the state in which the puzzles pieces begin, and the
// state in which they must be placed in order to solve the puzzle.
// 
// Each side of each cube is assigned a instance of a "Piece" class. A Piece
// represents a part of the face (holding which buddy it originated from, which face part it is,
// whether or not the piece must be in place to solve the puzzle and which attribute it has). 
// 
// For example: Piece(1, 2, true, ATTR_HIDDEN) would be the mouth of Buddy 1 (the blue cat),
// it is necessary to be in the right spot to solve the puzzle, but it is a tricky hidden piece!
// Must solve defaults to false and attributes default to ATTR_NONE.
//
// Piece(BuddyId, PartId, MustSolve, Attribute = ATTR_NONE)
// - BuddyId = [0...kNumCubes)
// - PartId = [0...NUM_SIDES)
// - MustSolve = true/false
////////////////////////////////////////////////////////////////////////////////////////////////////

Puzzle sPuzzleDefault;
Puzzle sPuzzles[4]; // TODO: Expand to 15 once I slimmify the data format.

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void InitializePuzzles()
{
    DEBUG_LOG(("sizeof(Piece) = %lu\n", sizeof(Piece)));
    DEBUG_LOG((
        "sizeof(Puzzle) = %lu x %lu = %lu\n",
        sizeof(Puzzle), arraysize(sPuzzles), sizeof(sPuzzles)));
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle Default (shoulnd't have to change this)
    ////////////////////////////////////////////////////////////////////////////////
    
    sPuzzleDefault.Reset();
    sPuzzleDefault.SetTitle("Default");
    sPuzzleDefault.AddCutsceneTextStart("[A]Default");
    sPuzzleDefault.AddCutsceneTextEnd("[B]Default");
    sPuzzleDefault.SetClue("Default");
    sPuzzleDefault.SetNumShuffles(0);
    
    for (unsigned int i = 0; i < kMaxBuddies; ++i)
    {
        sPuzzleDefault.AddBuddy(i);
        
        sPuzzleDefault.SetPieceStart( i, SIDE_TOP,    Piece(i, 0));
        sPuzzleDefault.SetPieceStart( i, SIDE_LEFT,   Piece(i, 1));
        sPuzzleDefault.SetPieceStart( i, SIDE_BOTTOM, Piece(i, 2));
        sPuzzleDefault.SetPieceStart( i, SIDE_RIGHT,  Piece(i, 3));
        
        sPuzzleDefault.SetPieceEnd(   i, SIDE_TOP,    Piece(i, 0, true));
        sPuzzleDefault.SetPieceEnd(   i, SIDE_LEFT,   Piece(i, 1, true));
        sPuzzleDefault.SetPieceEnd(   i, SIDE_BOTTOM, Piece(i, 2, true));
        sPuzzleDefault.SetPieceEnd(   i, SIDE_RIGHT,  Piece(i, 3, true));
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    
    unsigned int i = 0;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 0
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(i < arraysize(sPuzzles));
    
    sPuzzles[i].Reset();
    sPuzzles[i].SetTitle("Big Mouth");
    
    sPuzzles[i].AddCutsceneTextStart("[A]Gimme a kiss");
    sPuzzles[i].AddCutsceneTextStart("[B]Can I use\nyour mouth?");
    sPuzzles[i].AddCutsceneTextStart("[A]...");
    sPuzzles[i].AddCutsceneTextStart("[A]OK");
    
    sPuzzles[i].AddCutsceneTextEnd("[B]Muuuahhh");
    sPuzzles[i].AddCutsceneTextEnd("[A]Hot n'\nheavy!");
    
    sPuzzles[i].SetClue("Swap Mouths");
    sPuzzles[i].SetNumShuffles(0);
    
    sPuzzles[i].AddBuddy(0);
    sPuzzles[i].AddBuddy(1);
    
    sPuzzles[i].SetPieceStart( 0, SIDE_TOP,    Piece(0, 0));
    sPuzzles[i].SetPieceStart( 0, SIDE_LEFT,   Piece(0, 1));
    sPuzzles[i].SetPieceStart( 0, SIDE_BOTTOM, Piece(0, 2));
    sPuzzles[i].SetPieceStart( 0, SIDE_RIGHT,  Piece(0, 3));
    
    sPuzzles[i].SetPieceStart( 1, SIDE_TOP,    Piece(1, 0));
    sPuzzles[i].SetPieceStart( 1, SIDE_LEFT,   Piece(1, 1));
    sPuzzles[i].SetPieceStart( 1, SIDE_BOTTOM, Piece(1, 2));
    sPuzzles[i].SetPieceStart( 1, SIDE_RIGHT,  Piece(1, 3));
    
    sPuzzles[i].SetPieceEnd(   0, SIDE_TOP,    Piece(0, 0));
    sPuzzles[i].SetPieceEnd(   0, SIDE_LEFT,   Piece(0, 1));
    sPuzzles[i].SetPieceEnd(   0, SIDE_BOTTOM, Piece(1, 2, true));
    sPuzzles[i].SetPieceEnd(   0, SIDE_RIGHT,  Piece(0, 3));
    
    sPuzzles[i].SetPieceEnd(   1, SIDE_TOP,    Piece(1, 0));
    sPuzzles[i].SetPieceEnd(   1, SIDE_LEFT,   Piece(1, 1));
    sPuzzles[i].SetPieceEnd(   1, SIDE_BOTTOM, Piece(0, 2, true));
    sPuzzles[i].SetPieceEnd(   1, SIDE_RIGHT,  Piece(1, 3));   
    
    ++i;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 1
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(i < arraysize(sPuzzles));
    
    sPuzzles[i].Reset();
    sPuzzles[i].SetTitle("All Mixed Up");
    
    sPuzzles[i].AddCutsceneTextStart("[A]Let's get\nCRAZY!");
    sPuzzles[i].AddCutsceneTextEnd("[B]My head\nhurts.");
    
    sPuzzles[i].SetClue("Unscramble");
    sPuzzles[i].SetNumShuffles(3);
    
    sPuzzles[i].AddBuddy(1);
    sPuzzles[i].AddBuddy(2);
    
    sPuzzles[i].SetPieceStart( 0, SIDE_TOP,    Piece(1, 0));
    sPuzzles[i].SetPieceStart( 0, SIDE_LEFT,   Piece(1, 1));
    sPuzzles[i].SetPieceStart( 0, SIDE_BOTTOM, Piece(1, 2));
    sPuzzles[i].SetPieceStart( 0, SIDE_RIGHT,  Piece(1, 3));
    
    sPuzzles[i].SetPieceStart( 1, SIDE_TOP,    Piece(2, 0));
    sPuzzles[i].SetPieceStart( 1, SIDE_LEFT,   Piece(2, 1));
    sPuzzles[i].SetPieceStart( 1, SIDE_BOTTOM, Piece(2, 2));
    sPuzzles[i].SetPieceStart( 1, SIDE_RIGHT,  Piece(2, 3));
    
    sPuzzles[i].SetPieceEnd(   0, SIDE_TOP,    Piece(1, 0, true));
    sPuzzles[i].SetPieceEnd(   0, SIDE_LEFT,   Piece(1, 1, true));
    sPuzzles[i].SetPieceEnd(   0, SIDE_BOTTOM, Piece(1, 2, true));
    sPuzzles[i].SetPieceEnd(   0, SIDE_RIGHT,  Piece(1, 3, true));
    
    sPuzzles[i].SetPieceEnd(   1, SIDE_TOP,    Piece(2, 0, true));
    sPuzzles[i].SetPieceEnd(   1, SIDE_LEFT,   Piece(2, 1, true));
    sPuzzles[i].SetPieceEnd(   1, SIDE_BOTTOM, Piece(2, 2, true));
    sPuzzles[i].SetPieceEnd(   1, SIDE_RIGHT,  Piece(2, 3, true));  
    
    ++i;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 2
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(i < arraysize(sPuzzles));
    
    sPuzzles[i].Reset();
    sPuzzles[i].SetTitle("Bad Hair Day");
    
    sPuzzles[i].AddCutsceneTextStart("[A]How do I get\ncool hair");
    sPuzzles[i].AddCutsceneTextStart("[A]like you?");
    sPuzzles[i].AddCutsceneTextEnd("[A]Now I look\nlike Kelly!");
    
    sPuzzles[i].SetClue("Swap Hair");
    sPuzzles[i].SetNumShuffles(0);
    
    sPuzzles[i].AddBuddy(3);
    sPuzzles[i].AddBuddy(4);
    
    sPuzzles[i].SetPieceStart( 0, SIDE_TOP,    Piece(3, 0));
    sPuzzles[i].SetPieceStart( 0, SIDE_LEFT,   Piece(3, 1));
    sPuzzles[i].SetPieceStart( 0, SIDE_BOTTOM, Piece(3, 2));
    sPuzzles[i].SetPieceStart( 0, SIDE_RIGHT,  Piece(3, 3));
    
    sPuzzles[i].SetPieceStart( 1, SIDE_TOP,    Piece(4, 0));
    sPuzzles[i].SetPieceStart( 1, SIDE_LEFT,   Piece(4, 1));
    sPuzzles[i].SetPieceStart( 1, SIDE_BOTTOM, Piece(4, 2));
    sPuzzles[i].SetPieceStart( 1, SIDE_RIGHT,  Piece(4, 3));
    
    sPuzzles[i].SetPieceEnd(   0, SIDE_TOP,    Piece(4, 0, true));
    sPuzzles[i].SetPieceEnd(   0, SIDE_LEFT,   Piece(3, 1));
    sPuzzles[i].SetPieceEnd(   0, SIDE_BOTTOM, Piece(3, 2));
    sPuzzles[i].SetPieceEnd(   0, SIDE_RIGHT,  Piece(3, 3));
    
    sPuzzles[i].SetPieceEnd(   1, SIDE_TOP,    Piece(3, 0, true));
    sPuzzles[i].SetPieceEnd(   1, SIDE_LEFT,   Piece(4, 1));
    sPuzzles[i].SetPieceEnd(   1, SIDE_BOTTOM, Piece(4, 2));
    sPuzzles[i].SetPieceEnd(   1, SIDE_RIGHT,  Piece(4, 3));   
    
    ++i;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 3
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(i < arraysize(sPuzzles));
    
    sPuzzles[i].Reset();
    sPuzzles[i].SetTitle("Private Eyes");
    
    sPuzzles[i].AddCutsceneTextStart("[A]See the world\nfrom my eyes!");
    sPuzzles[i].AddCutsceneTextEnd("[A]That's much\nbetter.");
    
    sPuzzles[i].SetClue("Swap Eyes");
    sPuzzles[i].SetNumShuffles(0);
    
    sPuzzles[i].AddBuddy(5);
    sPuzzles[i].AddBuddy(0);
    
    sPuzzles[i].SetPieceStart( 0, SIDE_TOP,    Piece(5, 0));
    sPuzzles[i].SetPieceStart( 0, SIDE_LEFT,   Piece(5, 1));
    sPuzzles[i].SetPieceStart( 0, SIDE_BOTTOM, Piece(5, 2));
    sPuzzles[i].SetPieceStart( 0, SIDE_RIGHT,  Piece(5, 3));
    
    sPuzzles[i].SetPieceStart( 1, SIDE_TOP,    Piece(0, 0));
    sPuzzles[i].SetPieceStart( 1, SIDE_LEFT,   Piece(0, 1));
    sPuzzles[i].SetPieceStart( 1, SIDE_BOTTOM, Piece(0, 2));
    sPuzzles[i].SetPieceStart( 1, SIDE_RIGHT,  Piece(0, 3));
    
    sPuzzles[i].SetPieceEnd(   0, SIDE_TOP,    Piece(5, 0));
    sPuzzles[i].SetPieceEnd(   0, SIDE_LEFT,   Piece(0, 1, true));
    sPuzzles[i].SetPieceEnd(   0, SIDE_BOTTOM, Piece(5, 2));
    sPuzzles[i].SetPieceEnd(   0, SIDE_RIGHT,  Piece(0, 3, true));
    
    sPuzzles[i].SetPieceEnd(   1, SIDE_TOP,    Piece(0, 0));
    sPuzzles[i].SetPieceEnd(   1, SIDE_LEFT,   Piece(5, 1, true));
    sPuzzles[i].SetPieceEnd(   1, SIDE_BOTTOM, Piece(0, 2));
    sPuzzles[i].SetPieceEnd(   1, SIDE_RIGHT,  Piece(5, 3, true));   
    
    ++i;
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumPuzzles()
{
    return arraysize(sPuzzles);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle &GetPuzzleDefault()
{
    return sPuzzleDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle &GetPuzzle(unsigned int puzzleIndex)
{
    ASSERT(puzzleIndex < arraysize(sPuzzles));
    return sPuzzles[puzzleIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
