#pragma once

#include <sifteo.h>
#include "assets.gen.h"

#ifndef NUM_CUBES
#  define NUM_CUBES 3
#endif

extern Cube cubes[NUM_CUBES];

using namespace Sifteo;

#define CORO_PARAMS int mState;
#define CORO_RESET mState=0
#define CORO_BEGIN switch(mState) { case 0:;
#define CORO_YIELD mState=__LINE__; return; case __LINE__:;
#define CORO_END mState=-1;case -1:;}

Cube::Side InferDirection(Vec2 u);

bool InSpriteMode(Cube* c);
void EnterSpriteMode(Cube *c);
void SetSpriteImage(Cube *c, int id, int tile);
void HideSprite(Cube *c, int id);
void ResizeSprite(Cube *c, int id, int px, int py);
void MoveSprite(Cube *c, int id, int px, int py);