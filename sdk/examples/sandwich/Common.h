#pragma once

#include <sifteo.h>
#include "assets.gen.h"
#include "config.h"

extern Cube gCubes[NUM_CUBES];
//extern uint8_t gTouchFlags[NUM_CUBES];


// Audio Smutz
#if SFX_ON
extern AudioChannel gChannelSfx;
void PlaySfx(_SYSAudioModule& handle, bool preempt=true);
#else
#define PlaySfx(...)
#endif


#if MUSIC_ON
extern AudioChannel gChannelMusic;
void PlayMusic(_SYSAudioModule& music, bool loop=true);
#else
#define PlayMusic(...)
#endif

extern Math::Random gRandom;

using namespace Sifteo;

#define CORO_PARAMS int mState;
#define CORO_RESET mState=0
#define CORO_BEGIN switch(mState) { case 0:;
#define CORO_YIELD mState=__LINE__; return; case __LINE__:;
#define CORO_END mState=-1;case -1:;}

// Utils
Cube::Side InferDirection(Vec2 u);
