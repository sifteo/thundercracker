#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

#define VIEW_UNVISITED 0
#define VIEW_UNCHANGED 1
#define VIEW_CHANGED 2

static bool VisitMapView(uint8_t* visited, ViewSlot* view, Int2 loc, ViewSlot* origin=0) {
  if (!view || visited[view->GetCubeID()]) { return false; }
  if (origin) { view->GetCube()->orientTo(*(origin->GetCube())); }
  bool result = view->ShowLocation(loc, false, false);
  visited[view->GetCubeID()] = result ? VIEW_CHANGED:VIEW_UNCHANGED;
  for(Cube::Side i=0; i<NUM_SIDES; ++i) {
    result |= VisitMapView(visited, view->VirtualNeighborAt(i), loc+kSideToUnit[i].toInt(), view);
  }
  return result;
}

void Game::CheckMapNeighbors() {
  ViewSlot *root = mPlayer.View();
  if (!root->IsShowingRoom()) { return; }
  uint8_t visited[NUM_CUBES];
  for(unsigned i=0; i<NUM_CUBES; ++i) { visited[i] = 0; }
  bool chchchchanges = VisitMapView(visited, root, root->GetRoomView()->Location());
  
  if (chchchchanges) {
    PlaySfx(sfx_neighbor);
  }

  bool otherChanges = false;
  for(ViewSlot* v = ViewBegin(); v!=ViewEnd(); ++v) {
    if (!visited[v->GetCubeID()] && v->HideLocation(false)) { 
      otherChanges = true;
      visited[v->GetCubeID()] = VIEW_CHANGED;
    }
  }

  if (otherChanges && !chchchchanges) {
    PlaySfx(sfx_deNeighbor);    
  }
  
  sNeighborDirty = false;

  if (chchchchanges || otherChanges) {
    #if GFX_ARTIFACT_WORKAROUNDS
      Paint(true);
      for(ViewSlot *v=ViewBegin(); v!=ViewEnd(); ++v) {
        //if (visited[v->GetCubeID()] == VIEW_CHANGED) {
          v->GetCube()->vbuf.touch();
        //}
      }
      Paint(true);
      for(ViewSlot *v=ViewBegin(); v!=ViewEnd(); ++v) {
        //if (visited[v->GetCubeID()] == VIEW_CHANGED) {
          v->GetCube()->vbuf.touch();
        //}
      }
    #endif
    Paint(true);
  }


}
