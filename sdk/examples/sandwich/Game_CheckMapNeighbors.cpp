#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

struct VisitorStatus {
  uint32_t visitMask;
  uint32_t changeMask;
};

#define RESULT_OKAY         0
#define RESULT_INTERRUPTED  1

static unsigned VisitMapView(VisitorStatus* status, ViewSlot* view, Int2 loc, ViewSlot* origin=0, Cube::Side dir=0) {
  if (!view || (status->visitMask & view->GetCubeMask())) { 
    return RESULT_OKAY; 
  }
  if (origin) { 
    view->GetCube()->orientTo(*(origin->GetCube())); 
  }
  const bool didDisplayLocation = view->ShowLocation(loc, false, false);
  if (didDisplayLocation && view->ShowingRoom() && !view->ShowingLockedRoom()) {
    view->GetRoomView()->StartSlide((dir+2)%4);
  }
  status->visitMask |= view->GetCubeMask();
  if (didDisplayLocation) {
    status->changeMask |= view->GetCubeMask();
  }
  if (didDisplayLocation || !view->ShowingLockedRoom()) {
    for(Cube::Side i=0; i<NUM_SIDES; ++i) {
      const unsigned result = VisitMapView(
        status, 
        view->VirtualNeighborAt(i), 
        loc+kSideToUnit[i].toInt(), 
        view, 
        i
      );
      if (result != RESULT_OKAY) {
        return result;
      }
    }
  }
  return RESULT_OKAY;
}

void Game::CheckMapNeighbors() {
  mNeighborDirty = false;

  ViewSlot *root = mPlayer.View();
  if (!root->ShowingRoom()) { 
    return; 
  }
  
  VisitorStatus status;
  status.changeMask = 0x00000000;

  unsigned result;
  do {
    status.visitMask = 0x00000000;
    result = VisitMapView(&status, root, root->GetRoomView()->Location());
  } while(result != RESULT_OKAY);
  
  unsigned newChangeMask = 0;
  ViewSlot::Iterator i = ListViews(~status.visitMask);
  while(i.MoveNext()) {
    if (i->HideLocation(false)) {
      newChangeMask |= i->GetCubeMask();
    }
  }

  if (status.changeMask) {
    PlaySfx(sfx_neighbor);
  } else if (newChangeMask) {
    PlaySfx(sfx_deNeighbor);
  }

  if (newChangeMask || status.changeMask) {
    Paint(true);
  }


}
