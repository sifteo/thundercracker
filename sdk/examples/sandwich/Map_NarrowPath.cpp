#include "Map.h"
#include "Game.h"

struct ARecord {
  uint8_t tileID; // most significant bit identifies if this is open "0" or closed "1"
  uint8_t parentDirection; 
  uint8_t costToThis;
  uint8_t estimatedCostToDst;

  inline bool IsOpen() const { return (tileID & 0x80) == 0; }
  inline int Cost() const { return costToThis + estimatedCostToDst; }
};

struct ACell {
  uint8_t record;

  inline bool IsDefined() const { return record != 0xff; }
};

#define A_STAR_CAP (5*13)

struct AStar {
  Int2 offset;
  Int2 src;
  Int2 dst;
  int cellPitch;
  int cellRowCount;
  int recordCount;
  ARecord records[A_STAR_CAP];
  ACell cells[A_STAR_CAP];

  inline uint8_t TileToID(Int2 tile) const { return tile.x + cellPitch * tile.y; }
  inline Int2 IDToTile(uint8_t tid) const { tid &= (~0x80); return Vec2(tid % cellPitch, tid / cellPitch);}
  inline uint8_t Heuristic(Int2 tile) const { return abs(tile.x - dst.x) + abs(tile.y - dst.y); }
  void VisitNeighbor(ARecord* parent, Cube::Side dir);
};

void AStar::VisitNeighbor(ARecord* parent, Cube::Side dir) {
  Int2 tile = IDToTile(parent->tileID);
  Int2 ntile = tile + kSideToUnit[dir].toInt();
  if (ntile.x < 0 || ntile.y < 0 || ntile.x >= cellPitch || ntile.y >= cellRowCount) {
    return;
  }
  uint8_t nid = TileToID(ntile);
  if (cells[nid].IsDefined()) {
    ARecord *p = records + cells[nid].record;
    if (p->IsOpen()) {
      // is it cheaper to come from here?
      if (p->costToThis > parent->costToThis+1) {
        p->parentDirection = (dir+2)%4;
        p->costToThis = parent->costToThis+1;
      }
    }
  } else {
    // create a new record
    cells[nid].record = recordCount;
    ARecord *p = records + recordCount;
    p->tileID = nid;
    recordCount++;
    // is it walkable? (remember to convert normalized tile position to global tile position)
    if (gGame.GetMap()->IsVertexWalkable(ntile + Vec2(2,2) + (8 * offset))) {
      // open the record
      p->parentDirection = (dir+2)%4;
      p->costToThis = parent->costToThis + 1;
      p->estimatedCostToDst = Heuristic(ntile);
    } else {
      // close the record so noone else visits it
      p->tileID |= 0x80;
    }
  }    
}

bool Map::FindNarrowPath(BroadLocation bloc, Cube::Side dir, NarrowPath* outPath) {
  BroadLocation dbloc;
  if (!GetBroadLocationNeighbor(bloc, dir, &dbloc)) { return false; }
  Int2 loc = bloc.view->Location();
  Int2 dloc = loc + kSideToUnit[dir].toInt();
  Room* src = GetRoom(loc);
  Room* dst = GetRoom(dloc);
  
  AStar as;
  _SYS_memset8(&(as.cells->record), 0xff, A_STAR_CAP);

  //
  //  <WHAT>
  //
  // convert src/dst tile positions to normalized coordinates relative to the 65-tile pathfinding grid
  //as.offset = dir<2 ? dloc : loc;
  as.offset.x = dir<2 ? dloc.x : loc.x;
  as.offset.y = dir<2 ? dloc.y : loc.y;
  //
  // </WHAT>
  //

  as.src = src->LocalCenter(bloc.subdivision) + 8 * (loc - as.offset) - Vec2(2,2);
  as.dst = dst->LocalCenter(dbloc.subdivision) + 8 * (dloc - as.offset) - Vec2(2,2);
  as.cellPitch = dir % 2 == 0 ? 5 : 13; // vertical or horizontal?
  as.cellRowCount = dir % 2 == 0 ? 13 : 5; // vertical or horizontal?

  /*{
    // log what we *think* the map looks like
    LOG(("-------\n"));
    for(int row=0; row<as.cellRowCount; ++row) {
      for(int col=0; col<as.cellPitch; ++col) {
        Int2 tile = Vec2(col, row);
        if (tile == as.src) {
          LOG(("s"));
        } else if (tile == as.dst) {
          LOG(("d"));
        } else if (IsVertexWalkable(tile + Vec2(2,2) + (8 * as.offset))) {
          LOG(("."));
        } else {
          LOG(("W"));
        }
      }
      LOG(("\n"));
    }
  }*/

  // add an open record for src
  as.recordCount = 1;
  as.records->tileID = as.TileToID(as.src);
  as.records->parentDirection = 0;
  as.records->costToThis = 0;
  as.records->estimatedCostToDst = as.Heuristic(as.src);
  as.cells[as.records->tileID].record = 0;

  ARecord* pSelected;
  int iters = 0;
  do {
    iters++;
    // select the cheapest open node
    pSelected = 0;
    for(ARecord* p=as.records; p!=as.records+as.recordCount; ++p) {
      if (p->IsOpen() && (pSelected == 0 || p->Cost() < pSelected->Cost())) {
        pSelected = p;
      }
    }
    if (pSelected) {
      // get location and close the selected node
      Int2 tile = as.IDToTile(pSelected->tileID);
      pSelected->tileID |= 0x80;
      if (tile == as.dst) {
        // SUCCESS
        // fill in nodes backwards from dst to src
        outPath->pFirstMove = outPath->moves + PATH_CAPACITY;
        while(tile != as.src) {
          (outPath->pFirstMove)--;
          *(outPath->pFirstMove) = (pSelected->parentDirection + 2) % 4;
          tile += kSideToUnit[pSelected->parentDirection].toInt();
          pSelected = as.records + as.cells[as.TileToID(tile)].record;
        }
        return true;
      } else {
        // visit NSEW neighbors - first in target dir, then orth dirs, then opposite dir
        as.VisitNeighbor(pSelected, dir);
        as.VisitNeighbor(pSelected, (dir+1)%4);
        as.VisitNeighbor(pSelected, (dir+3)%4);
        as.VisitNeighbor(pSelected, (dir+2)%4);
      }
    }
  } while(pSelected);
  // WHAT IS THIS MADNESS?  Some bad data was exported
  ASSERT(false);
  return false;
}



