#include "Game.h"

void EdgeView::Init(int roomId, Cube::Side side) {
	mRoomId = roomId;
	mSide = side;
	// todo: compute screen orient
	ViewMode gfx = Parent()->Graphics();
	if (side < 4) {
		// todo: render gateway
		Room* pRoom = gGame.GetMap()->GetRoom(roomId);
		if (pRoom->HasGateway()) {
			for(int row=0; row<16; ++row)
			for(int col=0; col<16; ++col) {
				gfx.BG0_putTile(Vec2(col,row), *WhiteTile.tiles);
			}
		} else {
			// plot the top tiles
			for(int col=0; col<16; ++col) {
				gfx.BG0_putTile(Vec2(col,0), *EdgeTop.tiles);
			}
			for(int row=1; row<16; ++row)
			for(int col=0; col<16; ++col) {
				gfx.BG0_putTile(Vec2(col,row), *BlackTile.tiles);
			}
			Cube& c = *Parent()->GetCube();
			c.setOrientation((c.orientation() + 4 - side)%4);
		}
	} else {
		// plot the corner tile
		for(int row=0; row<16; ++row)
		for(int col=0; col<16; ++col) {
			gfx.BG0_putTile(Vec2(col,row), *BlackTile.tiles);
		}
		gfx.BG0_putTile(Vec2(0,0), *EdgeCorner.tiles);
	}
}

void EdgeView::Restore() {
	Init(mRoomId, mSide);
}

//void EdgeView::Update(float dt) {
//
//}