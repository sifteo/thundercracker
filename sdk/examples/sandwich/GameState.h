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
	bool FlagTrigger(const TriggerData& trigger);
	bool Flag(uint8_t questId, uint8_t flagId);
	bool AllQuestsComplete() { return mQuest == gQuestCount; }
	bool IsActive(const TriggerData& trigger) const;
	bool IsActive(uint8_t questId, uint8_t flagId) const;

private:
	void Save();
	void Load();
};