void InitializePuzzles()
{
	// Puzzle Default
	sPuzzleDefault.Reset();
	sPuzzleDefault.SetBook(0);
	sPuzzleDefault.SetTitle("Default");
	sPuzzleDefault.SetClue("Default");
	sPuzzleDefault.SetCutsceneEnvironment(0);
	sPuzzleDefault.AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "Default"));
	sPuzzleDefault.AddCutsceneLineEnd(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "Default"));
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
	sPuzzles[0].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "Gimme a kiss!"));
	sPuzzles[0].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_LEFT, CutsceneLine::POSITION_RIGHT, "Can I use your\nmouth?"));
	sPuzzles[0].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_FRONT, CutsceneLine::POSITION_LEFT, "..."));
	sPuzzles[0].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "OK!"));
	sPuzzles[0].AddCutsceneLineEnd(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "Muuuahhh!"));
	sPuzzles[0].AddCutsceneLineEnd(CutsceneLine(CutsceneLine::VIEW_LEFT, CutsceneLine::POSITION_RIGHT, "Hot n' heavy!"));
	sPuzzles[0].SetNumShuffles(0);
	sPuzzles[0].AddBuddy(BUDDY_GLUV);
	sPuzzles[0].SetPieceStart(0, SIDE_TOP, Piece(BUDDY_GLUV, Piece::PART_HAIR));
	sPuzzles[0].SetPieceStart(0, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH));
	sPuzzles[0].SetPieceStart(0, SIDE_RIGHT, Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT));
	sPuzzles[0].SetPieceStart(0, SIDE_LEFT, Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT));
	sPuzzles[0].SetPieceEnd(0, SIDE_TOP, Piece(BUDDY_GLUV, Piece::PART_HAIR, false));
	sPuzzles[0].SetPieceEnd(0, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH, true));
	sPuzzles[0].SetPieceEnd(0, SIDE_RIGHT, Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT, false));
	sPuzzles[0].SetPieceEnd(0, SIDE_LEFT, Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT, false));
	sPuzzles[0].AddBuddy(BUDDY_SULI);
	sPuzzles[0].SetPieceStart(1, SIDE_TOP, Piece(BUDDY_SULI, Piece::PART_HAIR));
	sPuzzles[0].SetPieceStart(1, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH));
	sPuzzles[0].SetPieceStart(1, SIDE_RIGHT, Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT));
	sPuzzles[0].SetPieceStart(1, SIDE_LEFT, Piece(BUDDY_SULI, Piece::PART_EYE_LEFT));
	sPuzzles[0].SetPieceEnd(1, SIDE_TOP, Piece(BUDDY_SULI, Piece::PART_HAIR, false));
	sPuzzles[0].SetPieceEnd(1, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH, true));
	sPuzzles[0].SetPieceEnd(1, SIDE_RIGHT, Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT, false));
	sPuzzles[0].SetPieceEnd(1, SIDE_LEFT, Piece(BUDDY_SULI, Piece::PART_EYE_LEFT, false));

	// Puzzle 1
	ASSERT(1 < arraysize(sPuzzles));
	sPuzzles[1].Reset();
	sPuzzles[1].SetBook(0);
	sPuzzles[1].SetTitle("All Mixed Up");
	sPuzzles[1].SetClue("Unscramble");
	sPuzzles[1].SetCutsceneEnvironment(0);
	sPuzzles[1].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "Let's get\nCRAZY!"));
	sPuzzles[1].AddCutsceneLineEnd(CutsceneLine(CutsceneLine::VIEW_LEFT, CutsceneLine::POSITION_RIGHT, "My head hurts."));
	sPuzzles[1].SetNumShuffles(3);
	sPuzzles[1].AddBuddy(BUDDY_RIKE);
	sPuzzles[1].SetPieceStart(0, SIDE_TOP, Piece(BUDDY_RIKE, Piece::PART_HAIR));
	sPuzzles[1].SetPieceStart(0, SIDE_BOTTOM, Piece(BUDDY_RIKE, Piece::PART_MOUTH));
	sPuzzles[1].SetPieceStart(0, SIDE_RIGHT, Piece(BUDDY_RIKE, Piece::PART_EYE_RIGHT));
	sPuzzles[1].SetPieceStart(0, SIDE_LEFT, Piece(BUDDY_RIKE, Piece::PART_EYE_LEFT));
	sPuzzles[1].SetPieceEnd(0, SIDE_TOP, Piece(BUDDY_RIKE, Piece::PART_HAIR, true));
	sPuzzles[1].SetPieceEnd(0, SIDE_BOTTOM, Piece(BUDDY_RIKE, Piece::PART_MOUTH, true));
	sPuzzles[1].SetPieceEnd(0, SIDE_RIGHT, Piece(BUDDY_RIKE, Piece::PART_EYE_RIGHT, true));
	sPuzzles[1].SetPieceEnd(0, SIDE_LEFT, Piece(BUDDY_RIKE, Piece::PART_EYE_LEFT, true));
	sPuzzles[1].AddBuddy(BUDDY_SULI);
	sPuzzles[1].SetPieceStart(1, SIDE_TOP, Piece(BUDDY_SULI, Piece::PART_HAIR));
	sPuzzles[1].SetPieceStart(1, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH));
	sPuzzles[1].SetPieceStart(1, SIDE_RIGHT, Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT));
	sPuzzles[1].SetPieceStart(1, SIDE_LEFT, Piece(BUDDY_SULI, Piece::PART_EYE_LEFT));
	sPuzzles[1].SetPieceEnd(1, SIDE_TOP, Piece(BUDDY_SULI, Piece::PART_HAIR, true));
	sPuzzles[1].SetPieceEnd(1, SIDE_BOTTOM, Piece(BUDDY_SULI, Piece::PART_MOUTH, true));
	sPuzzles[1].SetPieceEnd(1, SIDE_RIGHT, Piece(BUDDY_SULI, Piece::PART_EYE_RIGHT, true));
	sPuzzles[1].SetPieceEnd(1, SIDE_LEFT, Piece(BUDDY_SULI, Piece::PART_EYE_LEFT, true));

	// Puzzle 2
	ASSERT(2 < arraysize(sPuzzles));
	sPuzzles[2].Reset();
	sPuzzles[2].SetBook(1);
	sPuzzles[2].SetTitle("Bad Hair Day");
	sPuzzles[2].SetClue("Swap Hair");
	sPuzzles[2].SetCutsceneEnvironment(0);
	sPuzzles[2].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "How do I get\ncool hair\nlike you?"));
	sPuzzles[2].AddCutsceneLineEnd(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "Now I look\nlike Kelly!"));
	sPuzzles[2].SetNumShuffles(0);
	sPuzzles[2].AddBuddy(BUDDY_ZORG);
	sPuzzles[2].SetPieceStart(0, SIDE_TOP, Piece(BUDDY_ZORG, Piece::PART_HAIR));
	sPuzzles[2].SetPieceStart(0, SIDE_BOTTOM, Piece(BUDDY_ZORG, Piece::PART_MOUTH));
	sPuzzles[2].SetPieceStart(0, SIDE_RIGHT, Piece(BUDDY_ZORG, Piece::PART_EYE_RIGHT));
	sPuzzles[2].SetPieceStart(0, SIDE_LEFT, Piece(BUDDY_ZORG, Piece::PART_EYE_LEFT));
	sPuzzles[2].SetPieceEnd(0, SIDE_TOP, Piece(BUDDY_BOFF, Piece::PART_HAIR, true));
	sPuzzles[2].SetPieceEnd(0, SIDE_BOTTOM, Piece(BUDDY_ZORG, Piece::PART_MOUTH, false));
	sPuzzles[2].SetPieceEnd(0, SIDE_RIGHT, Piece(BUDDY_ZORG, Piece::PART_EYE_RIGHT, false));
	sPuzzles[2].SetPieceEnd(0, SIDE_LEFT, Piece(BUDDY_ZORG, Piece::PART_EYE_LEFT, false));
	sPuzzles[2].AddBuddy(BUDDY_BOFF);
	sPuzzles[2].SetPieceStart(1, SIDE_TOP, Piece(BUDDY_BOFF, Piece::PART_HAIR));
	sPuzzles[2].SetPieceStart(1, SIDE_BOTTOM, Piece(BUDDY_BOFF, Piece::PART_MOUTH));
	sPuzzles[2].SetPieceStart(1, SIDE_RIGHT, Piece(BUDDY_BOFF, Piece::PART_EYE_RIGHT));
	sPuzzles[2].SetPieceStart(1, SIDE_LEFT, Piece(BUDDY_BOFF, Piece::PART_EYE_LEFT));
	sPuzzles[2].SetPieceEnd(1, SIDE_TOP, Piece(BUDDY_ZORG, Piece::PART_HAIR, true));
	sPuzzles[2].SetPieceEnd(1, SIDE_BOTTOM, Piece(BUDDY_BOFF, Piece::PART_MOUTH, false));
	sPuzzles[2].SetPieceEnd(1, SIDE_RIGHT, Piece(BUDDY_BOFF, Piece::PART_EYE_RIGHT, false));
	sPuzzles[2].SetPieceEnd(1, SIDE_LEFT, Piece(BUDDY_BOFF, Piece::PART_EYE_LEFT, false));

	// Puzzle 3
	ASSERT(3 < arraysize(sPuzzles));
	sPuzzles[3].Reset();
	sPuzzles[3].SetBook(2);
	sPuzzles[3].SetTitle("Private Eyes");
	sPuzzles[3].SetClue("Swap Eyes");
	sPuzzles[3].SetCutsceneEnvironment(0);
	sPuzzles[3].AddCutsceneLineStart(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "See the world\nfrom my eyes!"));
	sPuzzles[3].AddCutsceneLineEnd(CutsceneLine(CutsceneLine::VIEW_RIGHT, CutsceneLine::POSITION_LEFT, "That's much\nbetter."));
	sPuzzles[3].SetNumShuffles(0);
	sPuzzles[3].AddBuddy(BUDDY_GLUV);
	sPuzzles[3].SetPieceStart(0, SIDE_TOP, Piece(BUDDY_GLUV, Piece::PART_HAIR));
	sPuzzles[3].SetPieceStart(0, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH));
	sPuzzles[3].SetPieceStart(0, SIDE_RIGHT, Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT));
	sPuzzles[3].SetPieceStart(0, SIDE_LEFT, Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT));
	sPuzzles[3].SetPieceEnd(0, SIDE_TOP, Piece(BUDDY_GLUV, Piece::PART_HAIR, false));
	sPuzzles[3].SetPieceEnd(0, SIDE_BOTTOM, Piece(BUDDY_GLUV, Piece::PART_MOUTH, false));
	sPuzzles[3].SetPieceEnd(0, SIDE_RIGHT, Piece(BUDDY_MARO, Piece::PART_EYE_RIGHT, true));
	sPuzzles[3].SetPieceEnd(0, SIDE_LEFT, Piece(BUDDY_MARO, Piece::PART_EYE_LEFT, true));
	sPuzzles[3].AddBuddy(BUDDY_MARO);
	sPuzzles[3].SetPieceStart(1, SIDE_TOP, Piece(BUDDY_MARO, Piece::PART_HAIR));
	sPuzzles[3].SetPieceStart(1, SIDE_BOTTOM, Piece(BUDDY_MARO, Piece::PART_MOUTH));
	sPuzzles[3].SetPieceStart(1, SIDE_RIGHT, Piece(BUDDY_MARO, Piece::PART_EYE_RIGHT));
	sPuzzles[3].SetPieceStart(1, SIDE_LEFT, Piece(BUDDY_MARO, Piece::PART_EYE_LEFT));
	sPuzzles[3].SetPieceEnd(1, SIDE_TOP, Piece(BUDDY_MARO, Piece::PART_HAIR, false));
	sPuzzles[3].SetPieceEnd(1, SIDE_BOTTOM, Piece(BUDDY_MARO, Piece::PART_MOUTH, false));
	sPuzzles[3].SetPieceEnd(1, SIDE_RIGHT, Piece(BUDDY_GLUV, Piece::PART_EYE_RIGHT, true));
	sPuzzles[3].SetPieceEnd(1, SIDE_LEFT, Piece(BUDDY_GLUV, Piece::PART_EYE_LEFT, true));

	sNumPuzzles = 4;
};

