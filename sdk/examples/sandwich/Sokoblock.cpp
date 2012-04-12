#include "Sokoblock.h"

void Sokoblock::Init(const SokoblockData& data) {
	mPosition.x = data.x;
	mPosition.y = data.y;
	mAssetId = data.asset;
}