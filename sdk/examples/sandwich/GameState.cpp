#include "GameState.h"
#include "config.h"

void GameState::Init() {
	mQuest = 0;
	mQuestMask = 0;
	mUnlockMask = 0;
	mItemSet = 0;//0xff
}

bool GameState::AdvanceQuest() {
	#if PLAYTESTING_HACKS
	mQuest = (mQuest + 1) % (gQuestCount-1);
	mQuestMask = 0;
	return true;
	#else
	if (mQuest < gQuestCount) {
		mQuest++;
		mQuestMask = 0;
		Save();
		return true;
	}
	return false;
	#endif
}

bool GameState::IsActive(const TriggerData& trigger) const {
	return (
		(trigger.questBegin == 0xff || trigger.questBegin <= mQuest) &&
		(trigger.questEnd == 0xff || trigger.questEnd >= mQuest) &&
		(
			trigger.flagId == 0 || 
			(trigger.flagId <= 32 && (mQuestMask & (1<<(trigger.flagId-1))) == 0) || 
			(trigger.flagId > 32 && (mUnlockMask & (1<<(trigger.flagId-33))) == 0)
		)
	);
}

bool GameState::IsActive(uint8_t questId, uint8_t flagId) const {
	return (
		(questId == 0xff || questId == mQuest) &&
		(
			flagId == 0 || 
			(flagId <= 32 && (mQuestMask & (1<<(flagId-1))) == 0) || 
			(flagId > 32 && (mUnlockMask & (1<<(flagId-33))) == 0)
		)
	);
}

bool GameState::FlagTrigger(const TriggerData& trigger) {
	if (IsActive(trigger) && trigger.flagId) {
		if (trigger.flagId <= 32) {
			mQuestMask |= (1 << (trigger.flagId-1));
		} else {
			mUnlockMask |= (1 << (trigger.flagId-33));
		}
		ASSERT(!IsActive(trigger));
		Save();
		return true;
	}
	return false;
}

bool GameState::Flag(uint8_t questId, uint8_t flagId) {
	if (IsActive(questId, flagId) && flagId) {
		if (flagId <= 32) {
			mQuestMask |= (1 << (flagId-1));
		} else {
			mUnlockMask |= (1 << (flagId-33));
		}
		ASSERT(!IsActive(questId, flagId));
		Save();
		return true;
	}
	return false;
}

bool GameState::PickupItem(int itemId) {
	ASSERT(gItemTypeData[itemId].storageType != STORAGE_EQUIPMENT);
	mItemSet |= (1<<itemId);
	ASSERT(HasItem(itemId));
	return true;
}

unsigned GameState::GetItems(uint8_t* buf) {
	unsigned result = 0;
	for(int8_t itemId=0; itemId<31; ++itemId) {
		if (HasItem(itemId)) { buf[result++] = itemId; }
	}
	return result;
}

void GameState::Save() {
	// TODO
}

void GameState::Load() {
	// TODO
}

