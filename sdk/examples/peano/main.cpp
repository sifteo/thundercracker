#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

//-----------------------------------------------------------------------------
// TYPES
//-----------------------------------------------------------------------------

struct ViewState {
  uint8_t masks[4];
};

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------

#ifndef NUM_CUBES
#  define NUM_CUBES 1
#endif

static Cube cubes[] = { Cube(0), Cube(1) };

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

static void RenderView(Cube& c, ViewState view) {
  // compute the screen state union (assuming it's valid)
  uint8_t vunion = view.masks[0] | view.masks[1] | view.masks[2] | view.masks[3];
  
  // flood fill the background to start (not optimal, but this is a demo, dogg)
  VidMode_BG0 mode(c.vbuf);
  mode.BG0_drawAsset(Vec2(0,0), Background);
  if (vunion == 0x00) { return; }
  
  // determine the "lowest" bit
  unsigned lowestBit = 0;
  while(!vunion & (1<<lowestBit)) { lowestBit++; }

  // Center
  for(unsigned i=5; i<9; ++i) {
    mode.BG0_drawAsset(Vec2(i,4), Center, lowestBit);
    mode.BG0_drawAsset(Vec2(i,9), Center, lowestBit);
  }
  for(unsigned i=5; i<9; ++i)
  for(unsigned j=4; j<10; ++j) {
    mode.BG0_drawAsset(Vec2(j,i), Center, lowestBit);
  }
  
  // Horizontal and Vertical
  
  
  
  // Major Joints
  // Major Diagonals
  // Minor Joints
  // Minor Diagonals
  
}

void siftmain() {
  for (unsigned i = 0; i < NUM_CUBES; i++) {
    cubes[i].enable();
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
  
  {
    ViewState view = { { 0x01, 0x00, 0x00, 0x00 }  };
    RenderView(cubes[0], view);
    
  }
  
  for(;;) {
    System::paint();
  }
}
