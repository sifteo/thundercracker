#pragma once
#include "View.h"

class EdgeView : View {
private:
	CORO_PARAMS;
	const GatewayData* mGateway;
	Dialog mDialog;
	uint8_t mRoomId;
	Cube::Side mSide;
	uint8_t t;

public:
	inline unsigned Id() const { return mRoomId;}
	inline Cube::Side Side() const { return mSide; }
	inline bool ShowingGateway() const { return mGateway!=0; }

	void Init(int roomId, Cube::Side side);
	void Restore();
	void Update(float dt);
};