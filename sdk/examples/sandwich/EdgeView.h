#pragma once
#include "View.h"

// additions to the regular side enum
#define SIDE_TOP_LEFT		4
#define SIDE_BOTTOM_LEFT	5
#define SIDE_BOTTOM_RIGHT	6
#define SIDE_TOP_RIGHT		7

class EdgeView : View {
private:
	uint8_t mRoomId;
	Cube::Side mSide;

public:
	unsigned Id() const { return mRoomId;}
	Cube::Side Side() const { return mSide; }

	void Init(int roomId, Cube::Side side);
	void Restore();
};