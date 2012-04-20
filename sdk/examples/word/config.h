#ifndef CONFIG_H
#define CONFIG_H

#include <sifteo.h>

const unsigned CUBE_ID_BASE = 0; //- start of the range of cube IDs
const unsigned NUM_CUBES = 3;
//  if cube IDs >= to CUBE_ALLOCATION asserts can happen during asset load,
// and bad things may generally happen. CUBE_ALLOCATION is currently 6,
// so if 3 games get unique cube IDs, it will need to be increased to at
// least the sum of all their NUM_CUBES
inline void compileMe() { STATIC_ASSERT(CUBE_ID_BASE + NUM_CUBES <= CUBE_ALLOCATION); }

const bool MUSIC_ON = true;
const bool SFX_ON = true;
const bool LOAD_ASSETS = true; //- do we load assets, or not?

#endif // CONFIG_H
