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

class Viewport {
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
    	unsigned currTouch	: 1;
    	unsigned prevTouch	: 1;
    	unsigned hasOverlay : 1;
  	} mFlags;
  	VideoBuffer mCanvas;

public:

	VideoBuffer& Canvas() { return mCanvas; }
	CubeID GetID() const { return mCanvas.cube(); }
	unsigned GetMask() const { return 1 << (31-GetID()); }
	bool Touched() const; // cube->touching && !prevTouch
	bool Active() const { return mFlags.view; }
	unsigned ViewType() const { return mFlags.view ; }
	bool ShowingRoom() const { return mFlags.view == VIEW_ROOM; }
	bool ShowingLockedRoom() const { return ShowingRoom() && mView.room.Locked(); }
	bool ShowingEdge() const { return mFlags.view == VIEW_EDGE; }
	bool ShowingGatewayEdge() const { return ShowingEdge() && mView.edge.ShowingGateway(); }
	bool ShowingLocation() const { return ShowingRoom() || ShowingEdge(); }
	IdleView& GetIdleView() { ASSERT(mFlags.view == VIEW_IDLE); return mView.idle; }
	RoomView& GetRoomView() { ASSERT(mFlags.view == VIEW_ROOM); return mView.room; }
	InventoryView& GetInventoryView() { ASSERT(mFlags.view == VIEW_INVENTORY); return mView.inventory; }
	MinimapView& GetMinimapView() { ASSERT(mFlags.view == VIEW_MINIMAP); return mView.minimap; }

	void Init();
	void Restore();
	void UpdateTouch();
	void Update();
  
	bool ShowLocation(Int2 location, bool force);
	bool HideLocation();

	void FlagOverlay() { mFlags.hasOverlay = true; }
	void RestoreCanonicalVram();
	void RefreshInventory();

	Side VirtualTiltDirection() const;
	Viewport* VirtualNeighborAt(Side side) const;

private:
	bool SetLocationView(unsigned roomId, Side side, bool force);
	void SetSecondaryView(unsigned viewId);
	void EvictSecondaryView(unsigned viewId);
	Viewport* FindIdleView();

public:
	class Iterator {
	private:
		friend class Game;
		unsigned mask;
		unsigned currentId;

		Iterator(unsigned setMask) : mask(setMask) {}
		Iterator() : mask(0x0), currentId(32) {}

	public:
		bool MoveNext() {
			//currentId = __builtin_clz(mask);
			currentId = fastclz(mask);
			mask ^= (0x80000000 >> currentId);
			return currentId < 32;
		}

		bool operator==(const Iterator& i) { return currentId == i.currentId; }
		bool operator!=(const Iterator& i) { return currentId != i.currentId; }

		Viewport& operator*();
		Viewport* operator->();
		Iterator operator++() { 
			MoveNext();
			return *this;
 		}

		operator Viewport*();
	};
};

extern Viewport *pInventory;
extern Viewport *pMinimap;
