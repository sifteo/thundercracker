#include "Game.h"

#define SPRITE_DOT_ID	0

#define mCanvas         (Parent().Canvas())  
#define mDotSprite		(Parent().Canvas().sprites[0])

void MinimapView::Init() {
	auto& map = gGame.GetMap();
	auto& data = map.Data();
	unsigned padLeft = (16 - data.width) >> 1;
	unsigned padTop = (16 - data.height) >> 1;

	// replace drawAsset w/ putTile?

	// fill in the top
	for(unsigned row=0; row<padTop; ++row) {
		for(unsigned col=0; col<18; ++col) {
			mCanvas.bg0.image(vec(col, row), BlackTile);
		}
	}

	// fill in the map
	for(unsigned row=0; row<data.height; ++row) {
		unsigned y = row + padTop;
		// fill in the left
		for(unsigned col=0; col<padLeft; ++col) {
			mCanvas.bg0.image(vec(col, y), BlackTile);
		}
		// fill in the map data
		for(unsigned col=0; col<data.width; ++col) {
			unsigned x = col + padLeft;
			mCanvas.bg0.image(vec(x,y), MinimapBasic, ComputeTileId(col, row));
		}

		// fill in the right
		for (unsigned col=padLeft+data.width; col<18; ++col) {
			mCanvas.bg0.image(vec(col, y), BlackTile);
		}
	}

	// fill in the bottom
	for(unsigned row=padTop+data.height; row<18; ++row) {
		for(unsigned col=0; col<18; ++col) {
			mCanvas.bg0.image(vec(col, row), BlackTile);
		}
	}

	Int2 pan = vec(-((data.width%2)<<2), -((data.height%2)<<2));
	mCanvas.bg0.setPanning(pan);

	mCanvasOffset.x = 8 * padLeft - pan.x - 4;
	mCanvasOffset.x = 8 * padTop - pan.y - 4;

	mDotSprite.setImage(MinimapDot);
	mDotSprite.move(
		(gGame.GetPlayer().Position()<<3) / 128 + mCanvasOffset.toInt()
	);
}

void MinimapView::Restore() {
	Init();
}

void MinimapView::Update() {
	mDotSprite.move(
		(gGame.GetPlayer().Position()<<3) / 128 + mCanvasOffset.toInt()
	);
}

unsigned MinimapView::ComputeTileId(int lx, int ly) {
	auto& map = gGame.GetMap();
	unsigned t = ly > 0 && map.GetPortalY(lx, ly-1);
	unsigned l = lx > 0 && map.GetPortalX(lx-1, ly);
	unsigned b = ly < map.Data().height-1 && map.GetPortalY(lx, ly);
	unsigned r = lx < map.Data().width-1 && map.GetPortalX(lx, ly);
	return (t) | (l<<1) | (b<<2) | (r<<3);
}