#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

struct VisitorStatus {
  uint32_t visitMask;
  uint32_t changeMask;
};

#define RESULT_OKAY         0
#define RESULT_INTERRUPTED  1

static unsigned VisitMapView(VisitorStatus* status, Viewport* view, Int2 loc, Viewport* origin=0, Side dir=NO_SIDE) {

  // Is it okay to visit this cube?
  if (!view || (status->visitMask & view->GetMask())) { 
    return RESULT_OKAY; 
  }
  status->visitMask |= view->GetMask();
  
  // Orient LCD to parent
  if (origin) { 
    // optimize precalc'd neighborhoods?
    view->Video().orientTo(origin->Video()); 
  }

  // Attempt to show location (returns true on change)
  const bool didDisplayLocation = view->ShowLocation(loc, false, false);
  if (didDisplayLocation) {
    status->changeMask |= view->GetMask();
  }

  // Start slide-out and possibly take over another view's lock
  if (didDisplayLocation && view->ShowingRoom() && !view->ShowingLockedRoom()) {
    view->GetRoomView()->StartSlide((Side)((dir+2)%4));
    // check this against locked views
    auto i = gGame.ListLockedViews();
    while(i.MoveNext()) {
      if (i->GetRoomView()->Id() == view->GetRoomView()->Id()) {
        view->GetRoomView()->Lock();
        i->GetRoomView()->Unlock();
        return RESULT_INTERRUPTED;
      }
    }
  }

  // Possibly make recursive calls
  if (didDisplayLocation || !view->ShowingLockedRoom()) {
    for(unsigned side=0; side<NUM_SIDES; ++side) {
      const unsigned result = VisitMapView(
        status, 
        view->VirtualNeighborAt((Side)side), 
        loc+Int2::unit(side), 
        view, 
        (Side)side
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
  Viewport *root = mPlayer.View();
  if (!root->ShowingRoom()) { 
    return; 
  }
  
  // will be used to track the status of the visitor as he
  // walks the "neighborhood tree"
  VisitorStatus status;
  status.changeMask = 0x00000000;

  // Keep walking the three until everything has been correctly visited
  // (some actions may cause us to have to start over becaues the tree is
  // modified as a side-effect)
  unsigned result;
  do {
    status.visitMask = 0x00000000;
    result = VisitMapView(&status, root, root->GetRoomView()->Location());
  } while(result != RESULT_OKAY);
  
  // Make sure all views outside the neighborhood are not showing rooms
  unsigned newChangeMask = 0;
  auto i = ListViews(~status.visitMask);
  while(i.MoveNext()) {
    if (i->HideLocation(false)) {
      newChangeMask |= i->GetMask();
    }
  }

  // Play SFX if the neighborhood changed
  if (status.changeMask) {
    PlaySfx(sfx_neighbor);
  } else if (newChangeMask) {
    PlaySfx(sfx_deNeighbor);
  }

  if (newChangeMask || status.changeMask) {
    DoPaint(true);
  }
}
