#pragma once

#include <sifteo.h>
#include "assets.gen.h"
//#include "audio.gen.h"
#include <stdlib.h>

#ifndef NUM_CUBES
#  define NUM_CUBES 3
#endif

//#define SKIP_INTRO
//#define SKIP_OUTRO

#ifndef SIFTEO_SIMULATOR
#define KLUDGES
#endif

extern Cube gCubes[NUM_CUBES];
extern AudioChannel gChannelSfx;
extern AudioChannel gChannelMusic;

using namespace Sifteo;

#define CORO_PARAMS int mState;
#define CORO_RESET mState=0
#define CORO_BEGIN switch(mState) { case 0:;
#define CORO_YIELD mState=__LINE__; return; case __LINE__:;
#define CORO_END mState=-1;case -1:;}

// Utils
Cube::Side InferDirection(Vec2 u);
unsigned int Rand( unsigned int max );

// Sprite Schmutz
bool InSpriteMode(Cube* c);
void EnterSpriteMode(Cube *c);
void SetSpriteImage(Cube *c, int id, int tile);
void HideSprite(Cube *c, int id);
void ResizeSprite(Cube *c, int id, int px, int py);
void MoveSprite(Cube *c, int id, int px, int py);

// Audio Smutz
void PlaySfx(_SYSAudioModule& handle, bool preempt=true);
void PlayMusic(_SYSAudioModule& music, bool loop=true);