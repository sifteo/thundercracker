#pragma once
#include "View.h"

class InventoryView : public View {
public:
	void Init();
	void Restore();
	void Update();

	void OnInventoryChanged();

private:
	void RenderInventory();
};
