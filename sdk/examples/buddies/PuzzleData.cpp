////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 - TRACER. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PuzzleData.h"
#include <sifteo/string.h>
#include "Piece.h"
#include "Puzzle.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// === Puzzle Data ===
//
// InitializePuzzles() is called on init and (surprise surprise) dynamically sets the data for
// all the puzzles. Most functions should be self-explantory, but pay special attention to the
// SetPieceStart and SetPieceEnd ones. These set the state in which the puzzles pieces begin,
// and the state in which they must be placed in order to solve the puzzle.
// 
// Each side of each cube is assigned a instance of a "Piece" class. A Piece
// represents a part of the face (holding which buddy it originated from, which face part it is,
// whether or not the piece must be in place to solve the puzzle and which attribute it has). 
// 
// For example: Piece(1, 2, true, ATTR_HIDDEN) would be the mouth of Buddy 1, it is necessary
// to be in the right spot to solve the puzzle, but it is a tricky hidden piece!
// "MustSolve" defaults to false and attributes default to ATTR_NONE.
//
// Piece(BuddyId, PartId, MustSolve, Attribute = ATTR_NONE)
// - BuddyId = [0...kMaxBuddies)
// - PartId = [0...NUM_SIDES)
// - MustSolve = true | false
// - Attribute = ATTR_NONE | ATTR_FIXED | ATTR_HIDDEN
//
// Also, note that attributes don't work right now until we figure out the sprites/BG1 issue.
//
// Additionally documentation is inline with Puzzle 0.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

Puzzle sPuzzleDefault;
Puzzle sPuzzles[kMaxBuddies * kPuzzlesPerBuddy];
unsigned int sNumPuzzles = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void InitializePuzzles()
{
    LOG(("sizeof(Piece) = %u\n", sizeof(Piece)));
    LOG((
        "sizeof(Puzzle) = %u x %u = %u\n",
        sizeof(Puzzle), arraysize(sPuzzles), sizeof(sPuzzles)));
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle Default (shoulnd't have to change this)
    ////////////////////////////////////////////////////////////////////////////////
    
    sPuzzleDefault.Reset();
    sPuzzleDefault.SetBook(0);
    sPuzzleDefault.SetTitle("Default");
    sPuzzleDefault.SetClue("Default");
    
    sPuzzleDefault.AddCutsceneTextStart("[A]Default");
    sPuzzleDefault.AddCutsceneTextEnd("[B]Default");
    
    sPuzzleDefault.SetNumShuffles(0);
    
    for (unsigned int i = 0; i < kMaxBuddies; ++i)
    {
        sPuzzleDefault.AddBuddy(BuddyId(i));
        
        sPuzzleDefault.SetPieceStart( i, SIDE_TOP,    Piece(BuddyId(i), Piece::PART_HAIR));
        sPuzzleDefault.SetPieceStart( i, SIDE_LEFT,   Piece(BuddyId(i), Piece::PART_EYE_LEFT));
        sPuzzleDefault.SetPieceStart( i, SIDE_BOTTOM, Piece(BuddyId(i), Piece::PART_MOUTH));
        sPuzzleDefault.SetPieceStart( i, SIDE_RIGHT,  Piece(BuddyId(i), Piece::PART_EYE_RIGHT));
        
        sPuzzleDefault.SetPieceEnd(   i, SIDE_TOP,    Piece(BuddyId(i), Piece::PART_HAIR, true));
        sPuzzleDefault.SetPieceEnd(   i, SIDE_LEFT,   Piece(BuddyId(i), Piece::PART_EYE_LEFT, true));
        sPuzzleDefault.SetPieceEnd(   i, SIDE_BOTTOM, Piece(BuddyId(i), Piece::PART_MOUTH, true));
        sPuzzleDefault.SetPieceEnd(   i, SIDE_RIGHT,  Piece(BuddyId(i), Piece::PART_EYE_RIGHT, true));
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    
    sNumPuzzles = 0;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 0
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(sNumPuzzles < arraysize(sPuzzles));
    
    sPuzzles[sNumPuzzles].Reset();
    sPuzzles[sNumPuzzles].SetBook(0);
    sPuzzles[sNumPuzzles].SetTitle("Big Mouth");
    sPuzzles[sNumPuzzles].SetClue("Swap Mouths");
    
    // Note:
    // - You can have up to 8 pages of text per cutscene
    // - Each page has 2 lines of 13 charateper
    // - Use \n to split lines
    // - Start text with "<" to denote the line is for the left speaker.
    // - Start text with ">" to denote the line is for the right speaker.
    sPuzzles[sNumPuzzles].AddCutsceneTextStart("<Gimme a kiss!");
    sPuzzles[sNumPuzzles].AddCutsceneTextStart(">Can I use your\nmouth?");
    sPuzzles[sNumPuzzles].AddCutsceneTextStart("<...");
    sPuzzles[sNumPuzzles].AddCutsceneTextStart("<OK!");
    sPuzzles[sNumPuzzles].AddCutsceneTextEnd(">Muuuahhh!");
    sPuzzles[sNumPuzzles].AddCutsceneTextEnd("<Hot n' heavy!");
    
    // Currently you can do up to 256 shuffles.
    sPuzzles[sNumPuzzles].SetNumShuffles(0);
    
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_GLUV); // BuddyIndex 0: Add Gluv as the first buddy
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_SULI); // BuddyIndex 1: Add Suli as the second buddy
    
    // Start Pieces
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_TOP,    Piece(BUDDY_GLUV, Piece::PART_HAIR)); // BuddyIndex, Side, Piece
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_LEFT,   Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_RIGHT,  Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT));
    
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_TOP,    Piece(BUDDY_SULI, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_LEFT,   Piece(BUDDY_SULI, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_RIGHT,  Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT));
    
    // End Pieces
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_TOP,    Piece(BUDDY_GLUV, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_LEFT,   Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_RIGHT,  Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT));
    
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_TOP,    Piece(BUDDY_SULI, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_LEFT,   Piece(BUDDY_SULI, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_RIGHT,  Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT));   
    
    ++sNumPuzzles;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 1
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(sNumPuzzles < arraysize(sPuzzles));
    
    sPuzzles[sNumPuzzles].Reset();
    sPuzzles[sNumPuzzles].SetBook(0);
    sPuzzles[sNumPuzzles].SetTitle("All Mixed Up");
    sPuzzles[sNumPuzzles].SetClue("Unscramble");
    
    sPuzzles[sNumPuzzles].AddCutsceneTextStart("<Let's get\nCRAZY!");
    sPuzzles[sNumPuzzles].AddCutsceneTextEnd(">My head hurts.");
    
    sPuzzles[sNumPuzzles].SetNumShuffles(3);
    
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_SULI);
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_RIKE);
    
    // Start Pieces
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_TOP,    Piece(BUDDY_SULI, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_LEFT,   Piece(BUDDY_SULI, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_RIGHT,  Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT));
    
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_TOP,    Piece(BUDDY_RIKE, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_LEFT,   Piece(BUDDY_RIKE, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_BOTTOM, Piece(BUDDY_RIKE, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_RIGHT,  Piece(BUDDY_RIKE, Piece::PART_EYE_RIGHT));
    
    // End Pieces
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_TOP,    Piece(BUDDY_SULI, Piece::PART_HAIR, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_LEFT,   Piece(BUDDY_SULI, Piece::PART_EYE_LEFT, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_RIGHT,  Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT, true));
    
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_TOP,    Piece(BUDDY_RIKE, Piece::PART_HAIR, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_LEFT,   Piece(BUDDY_RIKE, Piece::PART_EYE_LEFT, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_BOTTOM, Piece(BUDDY_RIKE, Piece::PART_MOUTH, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_RIGHT,  Piece(BUDDY_RIKE, Piece::PART_EYE_RIGHT, true));  
    
    ++sNumPuzzles;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 2
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(sNumPuzzles < arraysize(sPuzzles));
    
    sPuzzles[sNumPuzzles].Reset();
    sPuzzles[sNumPuzzles].SetBook(1);
    sPuzzles[sNumPuzzles].SetTitle("Bad Hair Day");
    sPuzzles[sNumPuzzles].SetClue("Swap Hair");
    
    sPuzzles[sNumPuzzles].AddCutsceneTextStart("<How do I get\ncool hair\nlike you?");
    sPuzzles[sNumPuzzles].AddCutsceneTextEnd("<Now I look\nlike Kelly!");
    
    sPuzzles[sNumPuzzles].SetNumShuffles(0);
    
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_BOFF);
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_ZORG);
    
    // Start Pieces
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_TOP,    Piece(BUDDY_BOFF, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_LEFT,   Piece(BUDDY_BOFF, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_BOTTOM, Piece(BUDDY_BOFF, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_RIGHT,  Piece(BUDDY_BOFF, Piece::PART_EYE_RIGHT));
    
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_TOP,    Piece(BUDDY_ZORG, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_LEFT,   Piece(BUDDY_ZORG, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_BOTTOM, Piece(BUDDY_ZORG, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_RIGHT,  Piece(BUDDY_ZORG, Piece::PART_EYE_RIGHT));
    
    // End Pieces
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_TOP,    Piece(BUDDY_ZORG, Piece::PART_HAIR, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_LEFT,   Piece(BUDDY_BOFF, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_BOTTOM, Piece(BUDDY_BOFF, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_RIGHT,  Piece(BUDDY_BOFF, Piece::PART_EYE_RIGHT));
    
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_TOP,    Piece(BUDDY_BOFF, Piece::PART_HAIR, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_LEFT,   Piece(BUDDY_ZORG, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_BOTTOM, Piece(BUDDY_ZORG, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_RIGHT,  Piece(BUDDY_ZORG, Piece::PART_EYE_RIGHT));   
    
    ++sNumPuzzles;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Puzzle 3
    ////////////////////////////////////////////////////////////////////////////////
    
    ASSERT(sNumPuzzles < arraysize(sPuzzles));
    
    sPuzzles[sNumPuzzles].Reset();
    sPuzzles[sNumPuzzles].SetBook(3);
    sPuzzles[sNumPuzzles].SetTitle("Private Eyes");
    sPuzzles[sNumPuzzles].SetClue("Swap Eyes");
    
    sPuzzles[sNumPuzzles].AddCutsceneTextStart("<See the world\nfrom my eyes!");
    sPuzzles[sNumPuzzles].AddCutsceneTextEnd("<That's much\nbetter.");
    
    sPuzzles[sNumPuzzles].SetNumShuffles(0);
    
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_MARO);
    sPuzzles[sNumPuzzles].AddBuddy(BUDDY_GLUV);
    
    // Start Pieces
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_TOP,    Piece(BUDDY_MARO, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_LEFT,   Piece(BUDDY_MARO, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_BOTTOM, Piece(BUDDY_MARO, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 0, SIDE_RIGHT,  Piece(BUDDY_MARO, Piece::PART_EYE_RIGHT));
    
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_TOP,    Piece(BUDDY_GLUV, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_LEFT,   Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceStart( 1, SIDE_RIGHT,  Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT));
    
    // End Pieces
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_TOP,    Piece(BUDDY_MARO, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_LEFT,   Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_BOTTOM, Piece(BUDDY_MARO, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceEnd(   0, SIDE_RIGHT,  Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT, true));
    
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_TOP,    Piece(BUDDY_GLUV, Piece::PART_HAIR));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_LEFT,   Piece(BUDDY_MARO, Piece::PART_EYE_LEFT, true));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH));
    sPuzzles[sNumPuzzles].SetPieceEnd(   1, SIDE_RIGHT,  Piece(BUDDY_MARO, Piece::PART_EYE_RIGHT, true));   
    
    ++sNumPuzzles;
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumPuzzles()
{
    return sNumPuzzles;
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
