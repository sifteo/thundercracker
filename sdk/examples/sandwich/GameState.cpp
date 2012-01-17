#include "GameState.h"

GameState::GameState() : mQuest(0), mQuestMask(0), mUnlockMask(0) {
}

void GameState::AdvanceQuest() {
	if (mQuest < gQuestCount) {
		mQuest++;
		Save();
	}
}

bool GameState::IsActive(const TriggerData& trigger) const {
	return (
		(trigger.questBegin == 0xff || trigger.questBegin <= mQuest) &&
		(trigger.questEnd == 0xff || trigger.questEnd >= mQuest) &&
		(
			trigger.flagId == 0 || 
			(trigger.flagId <= 32 && (mQuestMask & (1<<(trigger.flagId-1))) == 0) || 
			((mUnlockMask & (1<<(trigger.flagId-33))) == 0)
		)
	);
}

void GameState::DoTrigger(const TriggerData& trigger) {
	if (IsActive(trigger) && trigger.flagId) {
		if (trigger.flagId <= 32) {
			mQuestMask |= (1 << (trigger.flagId-1));
		} else {
			mUnlockMask |= (1 << (trigger.flagId-33));
		}
		Save();
	}
}

void GameState::Save() {
	// TODO
}

void GameState::Load() {
	// TODO
}

