#pragma once
#include "RoomView.h"
#include "IdleView.h"
#include "InventoryView.h"

#define VIEW_IDLE		0
#define VIEW_ROOM		1
#define VIEW_INVENTORY	2
#define VIEW_TYPE_COUNT	3
#define BITS_FOR_VIEW_TYPE 2 // invariant: 2 ^ BITS_FOR_VIEW_TYPE >= VIEW_TYPE_COUNT

class ViewSlot {
private:
	union { // modal views must be first in order to allow casting
		IdleView idle;
		RoomView room;
		InventoryView inventory;
	} mView;
  	struct {
    	unsigned view : BITS_FOR_VIEW_TYPE;
    	unsigned prevTouch : 1;
  	} mFlags;

public:

	Cube* GetCube() const;
	Cube::ID GetCubeID() const;
	ViewMode Graphics() const { return ViewMode(GetCube()->vbuf); }
	BG1Helper Overlay() const { return BG1Helper(*GetCube()); }
	bool Touched() const; // cube->touching && !prevTouch
	inline unsigned ViewType() const { return mFlags.view ; }
	inline bool IsShowingRoom() const { return mFlags.view == VIEW_ROOM; }
	inline IdleView* GetIdleView() { ASSERT(mFlags.view == VIEW_IDLE); return &(mView.idle); }
	inline RoomView* GetRoomView() { ASSERT(mFlags.view == VIEW_ROOM); return &(mView.room); }
	inline InventoryView* GetInventoryView() { ASSERT(mFlags.view == VIEW_INVENTORY); return &(mView.inventory); }

	void Init();
	void Restore();
	void Update();
  
  	void HideSprites();

	bool ShowLocation(Vec2 location);
	bool HideLocation();

	void ShowInventory();
	void RefreshInventory();

	Cube::Side VirtualTiltDirection() const;
	ViewSlot* VirtualNeighborAt(Cube::Side side) const;

};
