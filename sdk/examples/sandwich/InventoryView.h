#pragma once
#include "View.h"
#include "Dialog.h"

class InventoryView : public View {
private:
	CORO_PARAMS;
	Short2 mTilt;
	Short2 mAccum;
	
	Dialog mDialog;

	uint8_t mSelected; // really only [0,16]
	uint8_t mTouch; // really only a bool
	uint8_t mAnim; // really only [0,32]

public:
	void Init();
	void Restore();
	void Update(float dt);

	void OnInventoryChanged();

private:
	void RenderInventory();
	void ComputeHoveringIconPosition();
	Cube::Side UpdateAccum();
	bool UpdateTouch();
};
