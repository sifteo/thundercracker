/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(kGameAssets);

static Metadata M = Metadata()
    .title("One-Sheet SDK Example")
    .package("com.sifteo.sdk.onesheet", "1.0")
    .icon(kIcon)
    .cubeRange(0, CUBE_ALLOCATION);

void main() {
  static VideoBuffer vbuf[CUBE_ALLOCATION];
  for(CubeID cube : CubeSet::connected()) {
    vbuf[cube].attach(cube);
    vbuf[cube].initMode(BG0_SPR_BG1);
    vbuf[cube].bg0.image(vec(0,0), kBackground);
    for(int i=0; i<8; ++i)
      vbuf[cube].sprites[i].setImage(kStar);
  }
  Float2 rotor = vec(cos(M_TAU/8.f), sin(M_TAU/8.f));
  Float2 center = vec(64,64) - kStar.tileSize()/2;
  Float2 offset;
  for(;;) {
    float radians = 1.5f * SystemTime::now().uptime();
    offset.setPolar(radians, 32.f);
    for(int i=0; i<8; ++i) {
      for(CubeID cube : CubeSet::connected())
        vbuf[cube].sprites[i].move(center + offset);
      offset = offset.cmul(rotor);
    }
    System::paint();
  }
}