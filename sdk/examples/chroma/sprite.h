/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SPRITE_H
#define _SPRITE_H

#include <sifteo.h>

using namespace Sifteo;

void moveSprite(Cube &cube, int id, int x, int y);
void resizeSprite(Cube &cube, int id, int w, int h);
void setSpriteImage(Cube &cube, int id, const Sifteo::PinnedAssetImage &assetImage, int frame);

#endif