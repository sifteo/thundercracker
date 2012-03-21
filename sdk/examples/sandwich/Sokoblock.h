#pragma once
#include "Common.h"
#include "Content.h"

class Sokoblock {
private:

	UShort2 mPosition;
	uint8_t mAssetId;

public:
	void Init(const SokoblockData& data);

	const PinnedAssetImage& Asset() const { return *gSokoblockAssets[mAssetId]; }

	Int2 Position() const { return mPosition; }
	void SetPosition(Int2 p) { mPosition = p; }
	void Move(Int2 v) { mPosition += UShort2(v); }
};