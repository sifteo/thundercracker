void InitializePuzzles()
{
	sPuzzleDefault.Reset();
	sPuzzleDefault.SetBook(0);
	sPuzzleDefault.SetTitle("Default");
	sPuzzleDefault.SetClue("Default");
	sPuzzleDefault.SetCutsceneEnvironment(0);
	sPuzzleDefault.AddCutsceneTextStart("<Default");
	sPuzzleDefault.AddCutsceneTextStart(">Default");
	sPuzzleDefault.SetNumShuffles(0);
	for (unsigned int i = 0; i < 6; ++i)
	{
		sPuzzleDefault.AddBuddy(BuddyId(i));
		sPuzzleDefault.SetPieceStart(i, SIDE_TOP, Piece(BuddyId(i), Piece::PART_HAIR));
		sPuzzleDefault.SetPieceStart(i, SIDE_LEFT, Piece(BuddyId(i), Piece::PART_EYE_LEFT));
		sPuzzleDefault.SetPieceStart(i, SIDE_BOTTOM, Piece(BuddyId(i), Piece::PART_MOUTH));
		sPuzzleDefault.SetPieceStart(i, SIDE_RIGHT, Piece(BuddyId(i), Piece::PART_EYE_RIGHT));
		sPuzzleDefault.SetPieceEnd(i, SIDE_TOP, Piece(BuddyId(i), Piece::PART_HAIR, true));
		sPuzzleDefault.SetPieceEnd(i, SIDE_LEFT, Piece(BuddyId(i), Piece::PART_EYE_LEFT, true));
		sPuzzleDefault.SetPieceEnd(i, SIDE_BOTTOM, Piece(BuddyId(i), Piece::PART_MOUTH, true));
		sPuzzleDefault.SetPieceEnd(i, SIDE_RIGHT, Piece(BuddyId(i), Piece::PART_EYE_RIGHT, true));
	}

	// Puzzle 0
	ASSERT(0 < arraysize(sPuzzles));
	sPuzzles[0].Reset();
	sPuzzles[0].SetBook(0);
	sPuzzles[0].SetTitle("Big Mouth");
	sPuzzles[0].SetClue("Swap Mouths");
	sPuzzles[0].SetCutsceneEnvironment(0);
	sPuzzles[0].AddCutsceneTextStart("<Gimme a kiss!");
	sPuzzles[0].AddCutsceneTextStart(">Can I use your\nmouth?");
	sPuzzles[0].AddCutsceneTextStart("<...");
	sPuzzles[0].AddCutsceneTextStart("<OK!");
	sPuzzles[0].AddCutsceneTextEnd(">Muuuahhh!");
	sPuzzles[0].AddCutsceneTextEnd("<Hot n' heavy!");
	sPuzzles[0].SetNumShuffles(0);
	sPuzzles[0].AddBuddy(BUDDY_GLUV);
	sPuzzles[0].AddBuddy(BUDDY_SULI);
	sPuzzles[0].SetPieceStart(0, SIDE_TOP, Piece(BUDDY_GLUV, Piece::PART_HAIR));
	sPuzzles[0].SetPieceStart(0, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH));
	sPuzzles[0].SetPieceStart(0, SIDE_RIGHT, Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT));
	sPuzzles[0].SetPieceStart(0, SIDE_LEFT, Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT));
	sPuzzles[0].SetPieceStart(1, SIDE_TOP, Piece(BUDDY_SULI, Piece::PART_HAIR));
	sPuzzles[0].SetPieceStart(1, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH));
	sPuzzles[0].SetPieceStart(1, SIDE_RIGHT, Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT));
	sPuzzles[0].SetPieceStart(1, SIDE_LEFT, Piece(BUDDY_SULI, Piece::PART_EYE_LEFT));
	sPuzzles[0].SetPieceEnd(0, SIDE_TOP, Piece(BUDDY_GLUV, Piece::PART_HAIR, false));
	sPuzzles[0].SetPieceEnd(0, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH, true));
	sPuzzles[0].SetPieceEnd(0, SIDE_RIGHT, Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT, false));
	sPuzzles[0].SetPieceEnd(0, SIDE_LEFT, Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT, false));
	sPuzzles[0].SetPieceEnd(1, SIDE_TOP, Piece(BUDDY_SULI, Piece::PART_HAIR, false));
	sPuzzles[0].SetPieceEnd(1, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH, true));
	sPuzzles[0].SetPieceEnd(1, SIDE_RIGHT, Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT, false));
	sPuzzles[0].SetPieceEnd(1, SIDE_LEFT, Piece(BUDDY_SULI, Piece::PART_EYE_LEFT, false));

	sNumPuzzles = 1;
};

