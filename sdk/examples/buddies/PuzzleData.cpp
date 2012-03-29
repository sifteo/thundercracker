////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 - TRACER. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PuzzleData.h"
#include "Book.h"
#include "Puzzle.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const CutsceneLine kCutsceneLineDefault[] =
{
    CutsceneLine(),
};
const BuddyId kBuddiesDefault[] =
{
    BUDDY_GLUV,
    BUDDY_SULI,
    BUDDY_RIKE,
    BUDDY_BOFF,
    BUDDY_ZORG,
    BUDDY_MARO
};
const Piece kPiecesDefaultStart[][NUM_SIDES] =
{
    {
        Piece(BUDDY_GLUV, Piece::PART_HAIR),
        Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT),
        Piece(BUDDY_GLUV, Piece::PART_MOUTH),
        Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT),
    },
    {
        Piece(BUDDY_SULI, Piece::PART_HAIR),
        Piece(BUDDY_SULI, Piece::PART_EYE_LEFT),
        Piece(BUDDY_SULI, Piece::PART_MOUTH),
        Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT),
    },
    {
        Piece(BUDDY_RIKE, Piece::PART_HAIR),
        Piece(BUDDY_RIKE, Piece::PART_EYE_LEFT),
        Piece(BUDDY_RIKE, Piece::PART_MOUTH),
        Piece(BUDDY_RIKE, Piece::PART_EYE_RIGHT),
    },
    {
        Piece(BUDDY_BOFF, Piece::PART_HAIR),
        Piece(BUDDY_BOFF, Piece::PART_EYE_LEFT),
        Piece(BUDDY_BOFF, Piece::PART_MOUTH),
        Piece(BUDDY_BOFF, Piece::PART_EYE_RIGHT),
    },
    {
        Piece(BUDDY_ZORG, Piece::PART_HAIR),
        Piece(BUDDY_ZORG, Piece::PART_EYE_LEFT),
        Piece(BUDDY_ZORG, Piece::PART_MOUTH),
        Piece(BUDDY_ZORG, Piece::PART_EYE_RIGHT),
    },
    {
        Piece(BUDDY_MARO, Piece::PART_HAIR),
        Piece(BUDDY_MARO, Piece::PART_EYE_LEFT),
        Piece(BUDDY_MARO, Piece::PART_MOUTH),
        Piece(BUDDY_MARO, Piece::PART_EYE_RIGHT),
    },
};
const Piece kPiecesDefaultEnd[][NUM_SIDES] =
{
    {
        Piece(BUDDY_GLUV, Piece::PART_HAIR, true),
        Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT, true),
        Piece(BUDDY_GLUV, Piece::PART_MOUTH, true),
        Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT, true),
    },
    {
        Piece(BUDDY_SULI, Piece::PART_HAIR, true),
        Piece(BUDDY_SULI, Piece::PART_EYE_LEFT, true),
        Piece(BUDDY_SULI, Piece::PART_MOUTH, true),
        Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT, true),
    },
    {
        Piece(BUDDY_RIKE, Piece::PART_HAIR, true),
        Piece(BUDDY_RIKE, Piece::PART_EYE_LEFT, true),
        Piece(BUDDY_RIKE, Piece::PART_MOUTH, true),
        Piece(BUDDY_RIKE, Piece::PART_EYE_RIGHT, true),
    },
    {
        Piece(BUDDY_BOFF, Piece::PART_HAIR, true),
        Piece(BUDDY_BOFF, Piece::PART_EYE_LEFT, true),
        Piece(BUDDY_BOFF, Piece::PART_MOUTH, true),
        Piece(BUDDY_BOFF, Piece::PART_EYE_RIGHT, true),
    },
    {
        Piece(BUDDY_ZORG, Piece::PART_HAIR, true),
        Piece(BUDDY_ZORG, Piece::PART_EYE_LEFT, true),
        Piece(BUDDY_ZORG, Piece::PART_MOUTH, true),
        Piece(BUDDY_ZORG, Piece::PART_EYE_RIGHT, true),
    },
    {
        Piece(BUDDY_MARO, Piece::PART_HAIR, true),
        Piece(BUDDY_MARO, Piece::PART_EYE_LEFT, true),
        Piece(BUDDY_MARO, Piece::PART_MOUTH, true),
        Piece(BUDDY_MARO, Piece::PART_EYE_RIGHT, true),
    },
};
Puzzle sPuzzleDefault(
    "Default",
    "Default",
    kBuddiesDefault, arraysize(kBuddiesDefault),
    kCutsceneLineDefault, arraysize(kCutsceneLineDefault),
    kBuddiesDefault, arraysize(kBuddiesDefault),
    kCutsceneLineDefault, arraysize(kCutsceneLineDefault),
    0,
    kBuddiesDefault, arraysize(kBuddiesDefault),
    0,
    kPiecesDefaultStart,
    kPiecesDefaultEnd);

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PuzzleData_GENERATED.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle &GetPuzzleDefault()
{
    return sPuzzleDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumBooks()
{
    return arraysize(kPuzzles);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Book &GetBook(unsigned int bookIndex)
{
    ASSERT(bookIndex < arraysize(kBooks));
    return kBooks[bookIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: kill this function in favor of GetBook().mNumPuzzles
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumPuzzles(unsigned int bookIndex)
{
    ASSERT(bookIndex < arraysize(kBooks));
    return kBooks[bookIndex].mNumPuzzles;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const Puzzle &GetPuzzle(unsigned int bookIndex, unsigned int puzzleIndex)
{
    ASSERT(bookIndex < arraysize(kBookOffsets));
    unsigned int iPuzzle = kBookOffsets[bookIndex] + puzzleIndex;
    
    ASSERT(iPuzzle < arraysize(kPuzzles));
    return kPuzzles[iPuzzle];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
