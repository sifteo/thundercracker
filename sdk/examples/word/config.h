#ifndef CONFIG_H
#define CONFIG_H

#include <sifteo.h>

const unsigned CUBE_ID_BASE = 0; //- start of the range of cube IDs
const unsigned NUM_CUBES = 5;
inline void compileMe() { STATIC_ASSERT(CUBE_ID_BASE + NUM_CUBES <= CUBE_ALLOCATION); }

const bool MUSIC_ON = false;
const bool SFX_ON = false;
const bool LOAD_ASSETS = true; //- do we load assets, or not?

#endif // CONFIG_H
