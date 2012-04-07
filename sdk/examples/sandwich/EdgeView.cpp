#include "Game.h"
#include "MapHelpers.h"

void EdgeView::Init(int roomId, Cube::Side side) {
	CORO_RESET;
	mRoomId = roomId;
	mSide = side;
	side = (side+2) % 4; // flip side
	// todo: compute screen orient
	ViewMode gfx = Parent()->Graphics();
	static const Int2 start[8] = {
		vec(0,0), vec(0,0), vec(0, 15), vec(15,0),
	};
	static const Int2 delta[4] = {
		vec(1,0), vec(0,1), vec(1,0), vec(0,1)
	};
	Cube::Side gateSide = SIDE_UNDEFINED;
	Room* pRoom = gGame.GetMap()->GetRoom(roomId);
	if (pRoom->HasGateway()) {
		// compute which "side" of the room the gateway is on
		mGateway = pRoom->Gateway();
		gateSide = ComputeGateSide(mGateway);
		if (gateSide == mSide) {
			// render gateway special
			for(int row=0; row<16; ++row)
			for(int col=0; col<16; ++col) {
				gfx.BG0_putTile(vec(col, row), BlackTile.tiles[0]);
			}
			gfx.BG0_drawAsset(vec(6, 5), IconPress);
		}
	}
	if (gateSide != mSide) {
		// render a side edge view
		mGateway = 0;
		gfx.BG0_drawAsset(vec(0,0), Blank);
		Int2 p=start[side];
		for(int i=0; i<16; ++i) {
			gfx.BG0_putTile(p, Edge.tiles[side]);
			p += delta[side];
		}
	}
}

void EdgeView::Restore() {
	Init(mRoomId, mSide);
}

void EdgeView::Update(float dt) {
	if (!mGateway) { return; }

	CORO_BEGIN;

	gGame.NeedsSync();
	CORO_YIELD;
	#if GFX_ARTIFACT_WORKAROUNDS		
		gGame.NeedsSync();
		Parent()->GetCube()->vbuf.touch();
		CORO_YIELD;
		gGame.NeedsSync();
		Parent()->GetCube()->vbuf.touch();
		CORO_YIELD;
	#endif
	Parent()->Graphics().setWindow(80+16,128-80-16);
	mDialog.Init(Parent()->GetCube());
	mDialog.Erase();
	mDialog.Show("Touch to go to"); {
		const MapData& targetMap = gMapData[mGateway->targetMap];
		mDialog.Show(targetMap.name);
	}
	gGame.NeedsSync();
	Parent()->GetCube()->vbuf.touch();
	CORO_YIELD;
	for(t=0; t<16; t++) {
		Parent()->Graphics().setWindow(80+15-(t),128-80-15+(t));
		mDialog.SetAlpha(t<<4);
		CORO_YIELD;
	}
	mDialog.SetAlpha(255);
	CORO_END;
}