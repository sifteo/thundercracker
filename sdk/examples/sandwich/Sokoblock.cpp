#include "Sokoblock.h"

void Sokoblock::Init(const SokoblockData& data) {
	mPositionX = data.x;
	mPositionY = data.y;
	mAssetId = data.asset;
}