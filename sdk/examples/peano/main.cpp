#include <sifteo.h>
#include "assets.gen.h"

#include "Fraction.h"
#include "Puzzle.h"
#include "Token.h"

using namespace Sifteo;

enum Bits { // tmp
  B00000, B00001, B00010, B00011, B00100, B00101, B00110, B00111,
  B01000, B01001, B01010, B01011, B01100, B01101, B01110, B01111,
  B10000, B10001, B10010, B10011, B10100, B10101, B10110, B10111,
  B11000, B11001, B11010, B11011, B11100, B11101, B11110, B11111,
};

//-----------------------------------------------------------------------------
// TYPES
//-----------------------------------------------------------------------------

struct ViewState {
  uint8_t masks[4];
};

struct Connection {
  Cube::ID c0;
  Cube::ID c1;
  Cube::Side s0;
};

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------

#ifndef NUM_CUBES
#  define NUM_CUBES 5
#endif

static Cube cubes[NUM_CUBES];
static Connection connections[NUM_CUBES-1];
static unsigned connectionCount = 0;

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

static const uint8_t keyIndices[] = { 0, 1, 3, 5, 8, 10, 13, 16, 20, 22, 25, 28, 32, 35, 39, 43, 48, 50, 53, 56, 60, 63, 67, 71, 76, 79, 83, 87, 92, 96, 101, 106, };

static int CountBits(uint8_t mask) {
  unsigned result = 0;
  for(unsigned i=0; i<5; ++i) {
    if (mask & (1<<i)) { result++; }
  }
  return result;
}

static void RenderView(Cube& c, ViewState view) {
  // compute the screen state union (assuming it's valid)
  uint8_t vunion = view.masks[0] | view.masks[1] | view.masks[2] | view.masks[3];
  
  ASSERT(vunion < (1<<6));

  // flood fill the background to start (not optimal, but this is a demo, dogg)
  VidMode_BG0 mode(c.vbuf);
  mode.BG0_drawAsset(Vec2(0,0), Background);
  if (vunion == 0x00) { return; }
  
  // determine the "lowest" bit
  unsigned lowestBit = 0;
  while(!(vunion & (1<<lowestBit))) { lowestBit++; }

  // Center
  for(unsigned i=5; i<9; ++i) {
    mode.BG0_drawAsset(Vec2(i,4), Center, lowestBit);
    mode.BG0_drawAsset(Vec2(i,9), Center, lowestBit);
  }
  for(unsigned i=5; i<9; ++i)
  for(unsigned j=4; j<10; ++j) {
    mode.BG0_drawAsset(Vec2(j,i), Center, lowestBit);
  }
  
  // Horizontal
  mode.BG0_drawAsset(Vec2(3,0), Horizontal, view.masks[SIDE_TOP]);
  mode.BG0_drawAsset(Vec2(3,13), Horizontal, view.masks[SIDE_BOTTOM]);
  mode.BG0_drawAsset(Vec2(3,14), Horizontal, view.masks[SIDE_BOTTOM]);
  mode.BG0_drawAsset(Vec2(3,15), Horizontal, view.masks[SIDE_BOTTOM]);
  
  // Vertical
  mode.BG0_drawAsset(Vec2(0,3), Vertical, view.masks[SIDE_LEFT]);
  mode.BG0_drawAsset(Vec2(13,3), Vertical, view.masks[SIDE_RIGHT]);
  mode.BG0_drawAsset(Vec2(14,3), Vertical, view.masks[SIDE_RIGHT]);
  mode.BG0_drawAsset(Vec2(15,3), Vertical, view.masks[SIDE_RIGHT]);

  // Minor Diagonals
  if (vunion & 0x10) {
    mode.BG0_drawAsset(Vec2(2,2), MinorNW, 1);
    mode.BG0_drawAsset(Vec2(2,11), MinorSW, 1);
    mode.BG0_drawAsset(Vec2(11,11), MinorSE, 0);
    mode.BG0_drawAsset(Vec2(11,2), MinorNE, 0);
  }

  // Major Diagonals
  if (lowestBit > 1) {
    mode.BG0_drawAsset(Vec2(4,4), Center, lowestBit);
    mode.BG0_drawAsset(Vec2(4,9), Center, lowestBit);
    mode.BG0_drawAsset(Vec2(9,9), Center, lowestBit);
    mode.BG0_drawAsset(Vec2(9,4), Center, lowestBit);
  } else {
    mode.BG0_drawAsset(Vec2(4,4), MajorNW, vunion & 0x03);
    mode.BG0_drawAsset(Vec2(4,9), MajorSW, vunion & 0x03);
    mode.BG0_drawAsset(Vec2(9,9), MajorSE, 3 - (vunion & 0x03));
    mode.BG0_drawAsset(Vec2(9,4), MajorNE, 3 - (vunion & 0x03));
  }

  // Major Joints
  mode.BG0_drawAsset(Vec2(3,1), MajorN, keyIndices[vunion] + CountBits(vunion ^ view.masks[0]));
  mode.BG0_drawAsset(Vec2(1,3), MajorW, keyIndices[vunion] + CountBits(vunion ^ view.masks[1]));
  mode.BG0_drawAsset(Vec2(3,10), MajorS, 111 - keyIndices[vunion] - CountBits(vunion ^ view.masks[2]));
  mode.BG0_drawAsset(Vec2(10,3), MajorE, 111 - keyIndices[vunion] - CountBits(vunion ^ view.masks[3]));

}

static void RenderViews() {
  ViewState views[NUM_CUBES];
  for(unsigned i=0; i<NUM_CUBES; ++i) {
    views[i].masks[0] = 0;
    views[i].masks[1] = 0;
    views[i].masks[2] = 0;
    views[i].masks[3] = 0;
  }
  for(unsigned i=0; i<connectionCount; ++i) {
    Connection& c = connections[i];
    uint8_t mask = 1<<i;
    views[c.c0].masks[c.s0] |= mask;
    views[c.c1].masks[(c.s0+2)%4] = mask;
    for(unsigned j=0; j<=i; ++j) {
      Connection& cj = connections[j];
      for(unsigned s=0; s<4; ++s) {
        if (views[cj.c0].masks[s]) { views[cj.c0].masks[s] |= mask; }
        if (views[cj.c1].masks[s]) { views[cj.c1].masks[s] |= mask; }
      }
    }
  }
  for(unsigned i=0; i<NUM_CUBES; ++i) {
    RenderView(cubes[i], views[i]);
  }  
}

static void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {
  if (connectionCount == NUM_CUBES-1 || s0 != (s1+2)%4) {
    return;
  }
  // update connections list
  if (s0 < 2) {
    connections[connectionCount].c0 = c0;
    connections[connectionCount].c1 = c1;
    connections[connectionCount].s0 = s0;
  } else {
    connections[connectionCount].c0 = c1;
    connections[connectionCount].c1 = c0;
    connections[connectionCount].s0 = s1;
  }
  connectionCount++;
  RenderViews();
}

static void OnNeighborRem(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {
  Cube::ID c = s0 < 2 ? c0 : c1;
  Cube::ID oc = s0 < 2 ? c1 : c0;
  Cube::Side s = s0 < 2 ? s0 : s1;
  bool dirty = false;
  for(unsigned i=0; i<connectionCount; ++i) {
    Connection& cn = connections[i];
    if (cn.s0 == s && cn.c0 == c && cn.c1 == oc) {
      connectionCount = i;
      dirty = true;
      break;
    }
  }
  if (dirty) {
    RenderViews();
  }
}

void siftmain() {
  for (unsigned i = 0; i < NUM_CUBES; i++) {
    cubes[i].enable(i);
    cubes[i].loadAssets(GameAssets);
    VidMode_BG0_ROM rom(cubes[i].vbuf);
    rom.init();
    rom.BG0_text(Vec2(1,1), "Loading...");
  }
  for (;;) {
    bool done = true;
    for (unsigned i = 0; i < NUM_CUBES; i++) {
      VidMode_BG0_ROM rom(cubes[i].vbuf);
      rom.BG0_progressBar(Vec2(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
      done &= cubes[i].assetDone(GameAssets);
    }
    System::paint();
    if (done) break;
  }
  for (unsigned i = 0; i < NUM_CUBES; i++) {
    VidMode_BG0 mode(cubes[i].vbuf);
    mode.init();
    mode.BG0_drawAsset(Vec2(0,0), Background);
  }
  _SYS_vectors.neighborEvents.add = OnNeighborAdd;
  _SYS_vectors.neighborEvents.remove = OnNeighborRem;
  
  /*{
    ViewState view = { { B10111, B10111, B10110, B00000 }  };
    RenderView(cubes[0], view);
    
  }*/
  
  for(;;) {
    System::paint();
  }
}
