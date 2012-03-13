#include "Map.h"
#include "Game.h"

bool Map::CanTraverse(BroadLocation bloc, Cube::Side side) {
  // validate that the exit portal is reachable given the subdivision type, or else
  // check the portal buffer for the general case
  switch(bloc.view->GetRoom()->SubdivType()) {
    case SUBDIV_DIAG_POS:
      if (side == SIDE_TOP || side == SIDE_LEFT) { return bloc.subdivision == 0; }
      return bloc.subdivision == 1;
    case SUBDIV_DIAG_NEG:
      if (side == SIDE_TOP || side == SIDE_RIGHT) { return bloc.subdivision == 0; }
      return bloc.subdivision == 1;
    case SUBDIV_BRDG_HOR:
    case SUBDIV_BRDG_VER:
      if (side == SIDE_TOP || side == SIDE_BOTTOM) { return bloc.subdivision == 0; }
      return bloc.subdivision == 1;
    default:
      Vec2 loc = bloc.view->Location();
      switch(side) {
        case SIDE_TOP: return loc.y > 0 && GetPortalY(loc.x, loc.y-1);
        case SIDE_LEFT: return loc.x > 0 && GetPortalX(loc.x-1, loc.y);
        case SIDE_BOTTOM: return loc.y < mData->height-1 && GetPortalY(loc.x, loc.y);
        case SIDE_RIGHT: return loc.x < mData->width-1 && GetPortalX(loc.x, loc.y);
      }
  }
  return false;
}

bool Map::GetBroadLocationNeighbor(BroadLocation loc, Cube::Side side, BroadLocation* outNeighbor) {
  if (!CanTraverse(loc, side)) { return false; }
  ViewSlot* gv = loc.view->Parent()->VirtualNeighborAt(side);
  if (!gv || !gv->IsShowingRoom()) { return false; }
  outNeighbor->view = gv->GetRoomView();
  const Room* room = outNeighbor->view->GetRoom();
  switch(room->SubdivType()) {
    case SUBDIV_DIAG_POS:
      outNeighbor->subdivision = (side == SIDE_BOTTOM || side == SIDE_RIGHT) ? 0 : 1;
      break;
    case SUBDIV_DIAG_NEG:
      outNeighbor->subdivision = (side == SIDE_BOTTOM || side == SIDE_LEFT) ? 0 : 1;
      break;
    case SUBDIV_BRDG_HOR:
    case SUBDIV_BRDG_VER:
      outNeighbor->subdivision = (side == SIDE_TOP || side == SIDE_BOTTOM) ? 0 : 1;
      break;
    default:
      outNeighbor->subdivision = 0;
      break;
  }
  return true;
}

// TODO: stack alloc?
static uint8_t sVisitMask[NUM_CUBES];

BroadPath::BroadPath() {
  for(int i=0; i<2*NUM_CUBES; ++i) {
    steps[i] = -1;
  }
}

bool BroadPath::PopStep(BroadLocation newRoot, BroadLocation* outNext) {
  if (steps[0] == -1 || steps[1] == -1) {
    steps[0] = -1;
    return false;
  }
  for(int i=0; i<2*NUM_CUBES; ++i) { steps[i] = steps[i+1]; }
  steps[NUM_CUBES-2] = -1;
  if (*steps >= 0 && gGame.GetMap()->GetBroadLocationNeighbor(newRoot, *steps, outNext)) {
    return true;
  }
  steps[0] = -1;
  return false;
}

void BroadPath::Cancel() {
  steps[0] = -1;
}


static bool Visit(BroadPath* outPath, BroadLocation loc, Cube::Side side, int depth) {
  BroadLocation next;
  if (!gGame.GetMap()->GetBroadLocationNeighbor(loc, side, &next) || sVisitMask[next.view->Parent()->GetCubeID()] & (1<<next.subdivision)) {
    if (depth > 1) {
      ViewSlot *nextView = loc.view->Parent()->VirtualNeighborAt(side);
      if (nextView && nextView->IsShowingGatewayEdge() && nextView->Touched()) {
        outPath->steps[depth-1] = -1;
        return true;
      }
    }
    return false;
  }
  sVisitMask[next.view->Parent()->GetCubeID()] |= (1<<next.subdivision);
  if (next.view->Parent()->Touched()) {
    outPath->steps[depth] = -1;
    return true;
  } else {
    for(int side=0; side<NUM_SIDES; ++side) {
      outPath->steps[depth] = side;
      if (Visit(outPath, next, side, depth+1)) {
        return true;
      } else {
        outPath->steps[depth] = -1;
      }
    }
  }
  outPath->steps[depth] = -1;
  return false;
}

bool Map::FindBroadPath(BroadPath* outPath) {
  for(unsigned i=0; i<NUM_CUBES; ++i) { sVisitMask[i] = 0; }
  const BroadLocation* pRoot = gGame.GetPlayer()->Current();
  sVisitMask[pRoot->view->Parent()->GetCubeID()] = (1 << pRoot->subdivision);
  for(int side=0; side<NUM_SIDES; ++side) {
    outPath->steps[0] = side;
    if (Visit(outPath, *pRoot, side, 1)) {
      return true;
    }
  }
  outPath->steps[0] = -1;
  return false;
}

