#pragma once
#include "View.h"

class EdgeView : View {
private:
	CORO_PARAMS;
	const GatewayData* mGateway;
	Dialog mDialog;
	uint8_t mRoomId;
	Side mSide;
	uint8_t t;

public:
	unsigned Id() const { return mRoomId;}
	Side GetSide() const { return mSide; }
	bool ShowingGateway() const { return mGateway!=0; }
	const GatewayData* Gateway() const { return mGateway; }

	void Init(int roomId, enum Side side);
	void Restore();
	void Update();
};