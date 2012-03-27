////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 - TRACER. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PuzzleData.h"
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
const BuddyId kBuddiesDefault[] = { BUDDY_GLUV, BUDDY_GLUV, };
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
    0,
    "Default",
    "Default",
    kCutsceneLineDefault, arraysize(kCutsceneLineDefault),
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

unsigned int GetNumPuzzles()
{
    return arraysize(kPuzzles);
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
    ASSERT(puzzleIndex < arraysize(kPuzzles));
    return kPuzzles[puzzleIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
