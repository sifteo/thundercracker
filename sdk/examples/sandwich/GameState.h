#pragma once
#include "Content.h"

class GameState {
private:
	uint32_t mQuest;
	uint32_t mQuestMask;
	uint32_t mUnlockMask;

public:
	GameState();
	void AdvanceQuest();
	bool IsActive(const TriggerData& trigger) const;

private:
	void Save();
	void Load();
};