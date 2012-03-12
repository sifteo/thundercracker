#pragma once
#include "RoomView.h"
#include "IdleView.h"
#include "InventoryView.h"
#include "MinimapView.h"
#include "EdgeView.h"

#define VIEW_NONE		0
#define VIEW_IDLE		1
#define VIEW_ROOM		2
#define VIEW_INVENTORY	3
#define VIEW_MINIMAP	4
#define VIEW_EDGE		5
#define VIEW_TYPE_COUNT	6

class ViewSlot {
private:
	union { // modal views must be first in order to allow casting
		IdleView idle;
		RoomView room;
		InventoryView inventory;
		MinimapView minimap;
		EdgeView edge;
	} mView;
  	struct {
    	unsigned view 		: 3; // 2^bitCount <= VIEW_TYPE_COUNT
    	unsigned prevTouch	: 1;
  	} mFlags;

public:

	Cube* GetCube() const;
	Cube::ID GetCubeID() const;
	ViewMode Graphics() const { ASSERT(mFlags.view); return ViewMode(GetCube()->vbuf); }
	bool Touched() const; // cube->touching && !prevTouch
	bool Active() const { return mFlags.view; }
	inline unsigned ViewType() const { return mFlags.view ; }
	inline bool IsShowingRoom() const { return mFlags.view == VIEW_ROOM; }
	inline bool IsShowingEdge() const { return mFlags.view == VIEW_EDGE; }
	inline bool IsShowingLocation() const { return IsShowingRoom() || IsShowingEdge(); }
	inline IdleView* GetIdleView() { ASSERT(mFlags.view == VIEW_IDLE); return &(mView.idle); }
	inline RoomView* GetRoomView() { ASSERT(mFlags.view == VIEW_ROOM); return &(mView.room); }
	inline InventoryView* GetInventoryView() { ASSERT(mFlags.view == VIEW_INVENTORY); return &(mView.inventory); }
	inline MinimapView* GetMinimapView() { ASSERT(mFlags.view == VIEW_MINIMAP); return &(mView.minimap); }

	void Init();
	void Restore(bool doFlush=true);
	void Update(float dt);
  
  	void HideSprites();

	bool ShowLocation(Vec2 location, bool force, bool doFlush=true);
	bool HideLocation(bool doFlush=true);

	void RefreshInventory(bool doFlush=true);

	Cube::Side VirtualTiltDirection() const;
	ViewSlot* VirtualNeighborAt(Cube::Side side) const;

private:
	bool SetLocationView(unsigned roomId, Cube::Side side, bool force, bool doFlush);
	void SetSecondaryView(unsigned viewId, bool doFlush);
	void SanityCheckVram();
	void EvictSecondaryView(unsigned viewId, bool doFlush);
	ViewSlot* FindIdleView();
};

extern ViewSlot *pInventory;
extern ViewSlot *pMinimap;
