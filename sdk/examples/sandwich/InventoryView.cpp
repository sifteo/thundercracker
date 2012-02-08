#include "Game.h"

void InventoryView::Init() {
	Parent()->HideSprites();
	Parent()->Graphics().BG0_drawAsset(Vec2(0,0), InventoryBackground);
	RenderInventory();
}

void InventoryView::Restore() {
	Init();
}

void InventoryView::Update() {
	
}

void InventoryView::OnInventoryChanged() {
	RenderInventory();
}

void InventoryView::RenderInventory() {
	BG1Helper overlay = Parent()->Overlay();
	unsigned count = 0;
	const int pad = 24;
	const int innerPad = (128-pad-pad)/3;
	const int firstSandwichId = 2;
	const int sandwichTypeCount = 4;
	for(int itemId=firstSandwichId; itemId<firstSandwichId+sandwichTypeCount; ++itemId) {
		if (pGame->GetState()->HasItem(itemId)) {
		  const int x = count % 4;
		  const int y = count >> 2;
		  overlay.DrawAsset(Vec2(1 + (x<<2),1 + (y<<2)), Items, itemId-1);
		  count++;
		}
	}
	if (pGame->GetState()->HasBasicKey()) {
		const int x = count % 4;
		const int y = count >> 2;
		overlay.DrawAsset(Vec2(1 + (x<<2),1 + (y<<2)), Items, ITEM_BASIC_KEY-1);
		count++;
	}
	overlay.Flush();	
	pGame->NeedsSync();
}