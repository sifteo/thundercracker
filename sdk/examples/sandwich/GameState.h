#pragma once
#include "Content.h"

class GameState {
private:
	unsigned mQuest;
	unsigned mQuestMask;
	unsigned mUnlockMask;

public:
	GameState();
	void AdvanceQuest();

	bool IsActive(const TriggerData& trigger) const;

private:
	void Save();
	void Load();
};