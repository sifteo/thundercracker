#include "Game.h"

void InventoryView::Init() {
	CORO_RESET;
	mSelected = 0;

	Parent()->HideSprites();
	Parent()->Graphics().BG0_drawAsset(Vec2(0,0), InventoryBackground);
	RenderInventory();
	Parent()->GetCube()->vbuf.touch();

}

void InventoryView::Restore() {
	Init();
}

void InventoryView::Update() {
	CORO_BEGIN;
	for(;;) {
		CORO_YIELD;
	}

	CORO_END;
}

void InventoryView::OnInventoryChanged() {
	RenderInventory();
}

void InventoryView::RenderInventory() {
	BG1Helper overlay = Parent()->Overlay();
	const int pad = 24;
	const int innerPad = (128-pad-pad)/3;
	uint8_t items[16];
	unsigned count = pGame->GetState()->GetItems(items);
	for(unsigned i=0; i<count; ++i) {
		const int x = i % 4;
		const int y = i >> 2;
		overlay.DrawAsset(Vec2(1 + (x<<2),1 + (y<<2)), Items, items[i]-1);
	}
	overlay.Flush();	
	pGame->NeedsSync();
}