#pragma once
#include "Common.h"
#include "Content.h"

#define FUSE_DURATION (20*10)
#define RESULT_

class Room;

class Bomb {
private:
	const ItemData* pItem;
	unsigned mCountdown; // we use frame-based time instead of walltime, just in case

public:
	void Initialize(const ItemData* babomb);
	void OnPickup();
	void UpdateFuse();

	const ItemData* Item() const { return pItem; }
	Room* RespawnRoom() const;
	bool FuseLit() const { return mCountdown != 0; }
};
