#pragma once
#include "Common.h"
#include "Content.h"

class Sokoblock {
private:
	uint16_t mPositionX;
	uint16_t mPositionY;
	uint8_t mAssetId;

public:
	void Init(const SokoblockData& data);

	const PinnedAssetImage& Asset() const { return *gSokoblockAssets[mAssetId]; }

	Vec2 Position() const { return Vec2(mPositionX, mPositionY); }
	void SetPosition(Vec2 p) { mPositionX = p.x; mPositionY = p.y; }
	void Move(Vec2 v) { mPositionX += v.x; mPositionY += v.y; }
};