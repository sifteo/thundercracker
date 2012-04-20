#include "DrawingHelpers.h"
#include "Game.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const int8_t kHoverTable[HOVER_COUNT] = {
  0, 0, 1, 2, 2, 3, 3, 3, 
  4, 3, 3, 3, 2, 2, 1, 0, 
  -1, -1, -2, -3, -3, -4, 
  -4, -4, -4, -4, -4, -4, 
  -3, -3, -2, -1
};


#define BFF_FRAME_COUNT 4

namespace BffDir {
  enum ID { E, SE, S, SW, W, NW, N, NE };
}

static const Int2 sBffTable[] = {
  vec(1, 0),
  vec(1, 1),
  vec(0, 1),
  vec(-1, 1),
  vec(-1, 0),
  vec(-1, -1),
  vec(0, -1),
  vec(1, -1),
};

void ButterflyFriend::Randomize() {
  using namespace BffDir;
  dir = (uint8_t) gRandom.randrange(8);
  switch(dir) {
    case S:
      pos.x = 64 + (uint8_t) gRandom.randrange(128);
      pos.y = (uint8_t) gRandom.randrange(64);
      break;
    case SE:
      pos.x = (uint8_t) gRandom.randrange(64);
      pos.y = (uint8_t) gRandom.randrange(64);
      break;
    case E:
      pos.x = (uint8_t) gRandom.randrange(64);
      pos.y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case NE:
      pos.x = (uint8_t) gRandom.randrange(64);
      pos.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case N:
      pos.x = 64 + (uint8_t) gRandom.randrange(128);
      pos.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case NW:
      pos.x = 192 + (uint8_t) gRandom.randrange(64);
      pos.y = 192 + (uint8_t) gRandom.randrange(64);
      break;
    case W:
      pos.x = 192 + (uint8_t) gRandom.randrange(64);
      pos.y = 64 + (uint8_t) gRandom.randrange(128);
      break;
    case SW:
      pos.x = 192 + (uint8_t) gRandom.randrange(128);
      pos.y = (uint8_t) gRandom.randrange(64);
      break;
  }
}

void ButterflyFriend::Update() {
    // butterfly stuff
    pos += UByte2(sBffTable[dir]);
    // hack - assumes butterflies and items are not rendered on same cube
    frame = (frame + 1) % (BFF_FRAME_COUNT * 3);
    using namespace BffDir;
    switch(dir) {
      case S:
        if (pos.y > 196) { Randomize(); }
        break;
      case SW:
        if (pos.x < 60 || pos.y > 196) { Randomize(); }
        break;
      case W:
        if (pos.x < 60) { Randomize(); }
        break;
      case NW:
        if (pos.x < 60 || pos.y < 60) { Randomize(); }
        break;
      case N:
      if (pos.y < 60) { Randomize(); }
        break;
      case NE:
      if (pos.x > 196 || pos.y < 60) { Randomize(); }
        break;
      case E:
      if (pos.x > 196) { Randomize(); }
        break;
      case SE:
      if (pos.x > 196 || pos.y > 196) { Randomize(); }
        break;
    }
}

