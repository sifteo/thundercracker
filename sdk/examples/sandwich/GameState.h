#pragma once
#include "Content.h"

class GameState {
private:
	uint32_t mQuest;
	uint32_t mQuestMask;
	uint32_t mUnlockMask;
	uint32_t mKeyCount;
  	uint32_t mItemSet;

public:
	void Init();
	bool AdvanceQuest();
	
	bool FlagTrigger(const TriggerData& trigger);
	bool Flag(uint8_t questId, uint8_t flagId);
	
	bool AllQuestsComplete() { return mQuest == gQuestCount; }
	
	bool IsActive(const TriggerData& trigger) const;
	bool IsActive(uint8_t questId, uint8_t flagId) const;

	bool HasAnyItems() const { return mItemSet || mKeyCount > 0; }
  	bool PickupItem(int itemId);
  	bool HasBasicKey() const { return mKeyCount > 0; }
  	bool HasItem(int itemId) const { return (mItemSet & (1<<itemId)) != 0; }
 	bool DecrementBasicKeyCount();


private:
	void Save();
	void Load();
};