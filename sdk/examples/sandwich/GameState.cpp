#include "GameState.h"

GameState::GameState() : mQuest(0), mQuestMask(0), mUnlockMask(0) {
}

void GameState::AdvanceQuest() {
	if (mQuest < gQuestCount) {
		mQuest++;
	}
}

bool GameState::IsActive(const TriggerData& trigger) const {
	// range must match
	return (
		(trigger.questBegin == 0xff || trigger.questBegin <= mQuest) &&
		(trigger.questEnd == 0xff || trigger.questEnd >= mQuest) &&
		(
			trigger.flagId == 0 || 
			(trigger.flagId <= 32 && (mQuestMask & (1<<(trigger.flagId-1)) == 0)) || 
			(mUnlockMask & (1<<(trigger.flagId-33)) == 0)
		)
	);
}

void GameState::Save() {
	ASSERT(0);
}

void GameState::Load() {
	ASSERT(0);
}

