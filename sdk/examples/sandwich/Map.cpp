#include "Map.h"
#include "Game.h"


Map::Map() {
  SetData(gMapData[gQuestData->mapId]);
}

void Map::SetData(const MapData& map) { 
  if (mData != (MapData*)&map) {
    mData = (MapData*)&map; 

    const Room* pEnd = mRooms + (map.width*map.height);
    for(Room* p=mRooms; p!=pEnd; ++p) { p->Clear(); }

    // find active triggers
    for(const ItemData* p = mData->items; p!= mData->items + mData->itemCount; ++p) {
      if (pGame->state.IsActive(p->trigger)) {
        ASSERT(!mRooms[p->trigger.room].HasTrigger());
        mRooms[p->trigger.room].SetTrigger(TRIGGER_ITEM, &(p->trigger));
      }
    }
    for(const GatewayData* p = mData->gates; p != mData->gates + mData->gateCount; ++p) {
      if (pGame->state.IsActive(p->trigger)) {
        ASSERT(!mRooms[p->trigger.room].HasTrigger());
        mRooms[p->trigger.room].SetTrigger(TRIGGER_GATEWAY, &(p->trigger));
      }
    }
    for(const NpcData* p = mData->npcs; p != mData->npcs + mData->npcCount; ++p) {
      if (pGame->state.IsActive(p->trigger)) {
        ASSERT(!mRooms[p->trigger.room].HasTrigger());
        mRooms[p->trigger.room].SetTrigger(TRIGGER_NPC, &(p->trigger));
      }
    }
    for(const DoorData* p = mData->doors; p != mData->doors + mData->doorCount; ++p) {
      mRooms[p->roomId].SetDoor(p);
    }
  }
}

bool Map::CanTraverse(Vec2 loc, Cube::Side direction) const {
    switch(direction) {
      case SIDE_TOP:
        return loc.y > 0 && GetPortalY(loc.x, loc.y-1);
      case SIDE_LEFT:
        return loc.x > 0 && GetPortalX(loc.x-1, loc.y);
      case SIDE_BOTTOM:
        return loc.y < mData->height-1 && GetPortalY(loc.x, loc.y);
      case SIDE_RIGHT:
        return loc.x < mData->width-1 && GetPortalX(loc.x, loc.y);
  }
  return false;
}

bool Map::IsVertexWalkable(Vec2 vertex) {
  // convert global vertex into location / local vertex pair
  Vec2 loc = Vec2(vertex.x>>3, vertex.y>>3);
  if (vertex.x == 0 || loc.x < 0 || loc.x >= mData->width || loc.y < 0 || loc.y >= mData->height) {
    return false; // out of bounds
  }
  vertex -= 8 * loc;
  return IsTileOpen(loc, vertex) && (
    vertex.x == 0 ? // are we between rooms?
      IsTileOpen(Vec2(loc.x-1, loc.y), Vec2(7, vertex.y)) : 
      IsTileOpen(loc, Vec2(vertex.x-1, vertex.y))
  );

}

//-----------------------------------------------------------------------------
// A* Crud
//-----------------------------------------------------------------------------

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

inline int abs(int x) { return x < 0 ? -x : x; }

struct AStar {
  Vec2 offset;
  Vec2 src;
  Vec2 dst;
  int cellPitch;
  int cellRowCount;
  int recordCount;
  ARecord records[A_STAR_CAP];
  ACell cells[A_STAR_CAP];

  inline uint8_t TileToID(Vec2 tile) const { return tile.x + cellPitch * tile.y; }
  inline Vec2 IDToTile(uint8_t tid) const { tid &= (~0x80); return Vec2(tid % cellPitch, tid / cellPitch);}
  inline uint8_t Heuristic(Vec2 tile) const { return abs(tile.x - dst.x) + abs(tile.y - dst.y); }

  void VisitNeighbor(ARecord* parent, Cube::Side dir) {
    Vec2 tile = IDToTile(parent->tileID);
    Vec2 ntile = tile + kSideToUnit[dir];
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
      if (pGame->map.IsVertexWalkable(ntile + Vec2(2,2) + (8 * offset))) {
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
};

bool Map::FindPath(Vec2 loc, Cube::Side dir, MapPath* outPath) {
  if (!CanTraverse(loc, dir)) { return false; }
  Vec2 dloc = loc + kSideToUnit[dir];
  Room* src = GetRoom(loc);
  Room* dst = GetRoom(dloc);
  
  AStar as;
  for(ACell* p = as.cells; p != as.cells + A_STAR_CAP; ++p) { p->record = 0xff; }

  // convert src/dst tile positions to normalized coordinates relative to the 65-tile pathfinding grid
  as.offset = dir == SIDE_TOP || dir == SIDE_LEFT ? dloc : loc;
  as.src = src->LocalCenter() + 8 * (loc - as.offset) - Vec2(2,2);
  as.dst = dst->LocalCenter() + 8 * (dloc - as.offset) - Vec2(2,2);
  as.cellPitch = dir % 2 == 0 ? 5 : 13; // vertical or horizontal?
  as.cellRowCount = dir % 2 == 0 ? 13 : 5; // vertical or horizontal?

  {
    // log what we *think* the map looks like
    LOG(("-------\n"));
    for(int row=0; row<as.cellRowCount; ++row) {
      for(int col=0; col<as.cellPitch; ++col) {
        Vec2 tile = Vec2(col, row);
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
  }

  // add an open record for src
  as.recordCount = 1;
  as.records->tileID = as.TileToID(as.src);
  as.records->parentDirection = 0;
  as.records->costToThis = 0;
  as.records->estimatedCostToDst = as.Heuristic(as.src);
  as.cells[as.records->tileID].record = 0;

  ARecord* pSelected;
  //int iters = 0;
  do {
    //iters++;
    // select the cheapest open node
    pSelected = 0;
    for(ARecord* p=as.records; p!=as.records+as.recordCount; ++p) {
      if (p->IsOpen() && (pSelected == 0 || p->Cost() < pSelected->Cost())) {
        pSelected = p;
      }
    }
    if (pSelected) {
      // get location and close the selected node
      Vec2 tile = as.IDToTile(pSelected->tileID);
      pSelected->tileID |= 0x80;
      if (tile == as.dst) {
        // SUCCESS
        // fill in nodes backwards from dst to src
        outPath->pFirstMove = outPath->moves + PATH_CAPACITY;
        while(tile != as.src) {
          (outPath->pFirstMove)--;
          *(outPath->pFirstMove) = (pSelected->parentDirection + 2) % 4;
          tile += kSideToUnit[pSelected->parentDirection];
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
  /*
  // WHAT IS THIS MADNESS??
  // Just fill in the trivial-path and call it a day
  outPath->pFirstMove = outPath->moves + PATH_CAPACITY;
  if (dir % 2 == 0) {
    // up-down neighbors, do XY order, left-right neighbors do YX order
    if (as.dst.x != as.src.x) {
      if (as.dst.x > as.src.x) {
        for(int i=as.src.x; i<as.dst.x; ++i) {
          (outPath->pFirstMove)--;
          *(outPath->pFirstMove) = SIDE_RIGHT;
        }
      } else {
        for(int i=as.src.x; i>as.dst.x; --i) {
          (outPath->pFirstMove)--;
          *(outPath->pFirstMove) = SIDE_LEFT;
        }
      }
    }
  }
  if (as.dst.y != as.src.y) {
    if (as.dst.y > as.src.y) {
      for(int i=as.src.y; i<as.dst.y; ++i) {
        (outPath->pFirstMove)--;
        *(outPath->pFirstMove) = SIDE_BOTTOM;
      }
    } else {
      for(int i=as.src.y; i>as.dst.y; --i) {
        (outPath->pFirstMove)--;
        *(outPath->pFirstMove) = SIDE_TOP;
      }
    }
  }
  if (dir%2 == 1) {
    // up-down neighbors, do XY order
    if (as.dst.x != as.src.x) {
      if (as.dst.x > as.src.x) {
        for(int i=as.src.x; i<as.dst.x; ++i) {
          (outPath->pFirstMove)--;
          *(outPath->pFirstMove) = SIDE_RIGHT;
        }
      } else {
        for(int i=as.src.x; i>as.dst.x; --i) {
          (outPath->pFirstMove)--;
          *(outPath->pFirstMove) = SIDE_LEFT;
        }
      }
    }
  }
  */
  return false;
}



