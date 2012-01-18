#pragma once
#include "Content.h"

class GameState {
private:
	uint32_t mQuest;
	uint32_t mQuestMask;
	uint32_t mUnlockMask;

public:
	GameState();
	bool AdvanceQuest();
	bool DoTrigger(const TriggerData& trigger);
	bool AllQuestsComplete() { return mQuest == gQuestCount; }
	bool IsActive(const TriggerData& trigger) const;

private:
	void Save();
	void Load();
};