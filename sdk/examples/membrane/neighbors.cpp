/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <stdlib.h>
#include <sifteo.h>

#include "game.h"


struct NeighborPair {
    int8_t side0;
    int8_t side1;
  
    bool FullyConnected() const { return side0 >= 0 && side1 >= 0; }
    bool FullyDisconnected() const { return side1 < 0 && side1 < 0; }
  
    NeighborPair* Lookup(int cid0, int cid1) {
        // invariant cid0 < cid1
        return (this + cid0 * (NUM_CUBES-1) + (cid1-1));
    }
};

#define HAS_NEIGHBOR(buf, cid, side) (buf[4*side]>>7)

void Game::checkNeighbors()
{
    // hacky  / for now - probably balls slow
    // coalesce pairs

    NeighborPair pairs[NUM_CUBES*NUM_CUBES];
    bool haveRemoves = false;
    
    for (NeighborPair *p = pairs; p != pairs + (NUM_CUBES-1)*(NUM_CUBES-1); ++p) {
        p->side0 = -1;
        p->side1 = -1;
    }

    uint8_t buf[NUM_CUBES*NUM_SIDES];
    for (int id=0; id<NUM_CUBES; ++id) {
        uint8_t* p = buf + id*NUM_SIDES;
        _SYS_getRawNeighbors(id, p);
    
        for (int side=0; side<NUM_SIDES; ++side) {
            if (p[side]>>7) {
                uint8_t otherId = p[side] & 0x1f;
                if (otherId < id) {
                    pairs->Lookup(otherId, id)->side1 = (int8_t)side;
                } else if (otherId < NUM_CUBES) {
                    pairs->Lookup(id, otherId)->side0 = (int8_t)side;
                }
            }
        }
    }

    // look for removes
    for (int id=0; id<NUM_CUBES; ++id) {
        GameCube &gc = getGameCube(id);
        
        for (int side=0; side<NUM_SIDES; ++side) {
            int otherId = gc.getNeighbor(side);
            
            if (otherId != INVALID_ID) {
                NeighborPair* p;

                if (id < otherId) {
                    p = pairs->Lookup(id, otherId);
                } else {
                    p = pairs->Lookup(otherId, id);
                }
                
                if (p->FullyDisconnected() && gc.getNeighbor(p->side0) != INVALID_ID) {
                    gc.setNeighbor(side, INVALID_ID);
                    haveRemoves = true;
                }
            }
        }
    }

    // look for adds
    for (unsigned id=0; id<NUM_CUBES-1; ++id) {
        GameCube &gc = getGameCube(id);

        for (unsigned otherId=id+1; otherId<NUM_CUBES; ++otherId) {
            NeighborPair* p = pairs->Lookup(id, otherId);
      
            if (p->FullyConnected()) {
                if (gc.getNeighbor(p->side0) == INVALID_ID) {
                    /*
                     * This is a new neighbor connection!
                     *
                     * Set the state (and trigger our portal animation) here,
                     * and this is where we actually figure out which
                     * particles should pass through the portal.
                     */
                
                    GameCube &other = getGameCube(otherId);
                
                    gc.setNeighbor(p->side0, otherId);
                    other.setNeighbor(p->side1, id);
                    
                    PortalPair pair = { &gc, &other, p->side0, p->side1 };
                    for (unsigned p = 0; p < NUM_PARTICLES; p++)
                        particles[p].portalNotify(pair);
                }
            }
        }
    }
    
    /*
     * When a cube is removed, that's when we check for matches. This lets you get a fourth
     * match if you can do it without removing a cube first.
     */
    if (haveRemoves)
        checkMatches();
}
