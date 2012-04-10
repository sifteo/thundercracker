#include "Bomb.h"
#include "Game.h"

void Bomb::Initialize(const ItemData* babomb) {
	ASSERT(gItemTypeData[babomb->itemId].triggerType == ITEM_TRIGGER_BOMB);
	pItem = babomb;
	mCountdown = 0;
}

Room* Bomb::RespawnRoom() const {
	return gGame.GetMap()->GetRoom(pItem->trigger.room);
}

void Bomb::OnPickup() {
	if (!mCountdown) {
		mCountdown = FUSE_DURATION;
	}
}

void Bomb::UpdateFuse() {
	mCountdown--;
}
