#include "Game.h"

void MinimapView::Init() {
	Parent()->HideSprites();
	BG1Helper(*Parent()->GetCube()).Flush();
	ViewMode g = Parent()->Graphics();
	Map *pMap = pGame->GetMap();
	const MapData* pData = pMap->Data();
	unsigned padLeft = (16 - pData->width) >> 1;
	unsigned padTop = (16 - pData->height) >> 1;

	// replace drawAsset w/ putTile?

	// fill in the top
	for(unsigned row=0; row<padTop; ++row) {
		for(unsigned col=0; col<18; ++col) {
			g.BG0_drawAsset(Vec2(col, row), Black);
		}
	}

	// fill in the map
	for(unsigned row=0; row<pData->height; ++row) {
		unsigned y = row + padTop;
		// fill in the left
		for(unsigned col=0; col<padLeft; ++col) {
			g.BG0_drawAsset(Vec2(col, y), Black);
		}
		// fill in the map data
		for(unsigned col=0; col<pData->width; ++col) {
			unsigned x = col + padLeft;
			g.BG0_drawAsset(Vec2(x,y), MinimapBasic, ComputeTileId(col, row));
		}

		// fill in the right
		for (unsigned col=padLeft+pData->width; col<18; ++col) {
			g.BG0_drawAsset(Vec2(col, y), Black);
		}
	}

	// fill in the bottom
	for(unsigned row=padTop+pData->height; row<18; ++row) {
		for(unsigned col=0; col<18; ++col) {
			g.BG0_drawAsset(Vec2(col, row), Black);
		}
	}
	g.BG0_setPanning(Vec2(-((pData->width%2)<<2), -((pData->height%2)<<2)));
}

void MinimapView::Restore() {
	Init();
}

void MinimapView::Update(float dt) {

}

unsigned MinimapView::ComputeTileId(int lx, int ly) {
	Map *pMap = pGame->GetMap();
	unsigned t = ly > 0 && pMap->GetPortalY(lx, ly-1);
	unsigned l = lx > 0 && pMap->GetPortalX(lx-1, ly);
	unsigned b = ly < pMap->Data()->height-1 && pMap->GetPortalY(lx, ly);
	unsigned r = lx < pMap->Data()->width-1 && pMap->GetPortalX(lx, ly);
	return (t) | (l<<1) | (b<<2) | (r<<3);
}