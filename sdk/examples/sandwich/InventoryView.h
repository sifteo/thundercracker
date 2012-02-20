#pragma once
#include "View.h"

class InventoryView : public View {
private:
	CORO_PARAMS;
	unsigned mSelected;

public:
	void Init();
	void Restore();
	void Update();

	void OnInventoryChanged();

private:
	void RenderInventory();
};
