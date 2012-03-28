/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Handle bubble sprites
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _BUBBLES_H
#define _BUBBLES_H

#include <sifteo.h>

using namespace Sifteo;

class CubeWrapper;

class Bubble
{
public:
    static const int SPAWN_EXTENT = 32;
    static const float BUBBLE_LIFETIME;
    static const float TILT_VEL;
    static const float BEHIND_CHROMITS_THRESHOLD;
    //at this depth, bubbles will move away from chromits
    static const float CHROMITS_COLLISION_DEPTH;
    static const float CHROMIT_OBSCURE_DIST;
    static const float CHROMIT_OBSCURE_DIST_2;

    Bubble();
    void Spawn();
    void Disable();
    void Update( float dt, const Float2 &tilt );
    void Draw( VidMode_BG0_SPR_BG1 &vid, int index, CubeWrapper *pWrapper );
    inline bool isAlive() const { return m_fTimeAlive >= 0.0f; }

private:
    void NormalizeVec( Float2 &vec );

    Float2 m_pos;
    float m_fTimeAlive;
    const Sifteo::PinnedAssetImage *m_pTex;
};

class BubbleSpawner
{
public:
    static const unsigned int MAX_BUBBLES = 4;
    static const float MIN_SHORT_SPAWN;
    static const float MAX_SHORT_SPAWN;
    static const float MIN_LONG_SPAWN;
    static const float MAX_LONG_SPAWN;
    static const float SHORT_PERIOD_CHANCE;
    static const unsigned int BUBBLE_SPRITEINDEX = 4;

    BubbleSpawner( VidMode_BG0_SPR_BG1 &vid );
    void Reset( VidMode_BG0_SPR_BG1 &vid );
    void Update( float dt, const Float2 &tilt );
    void Draw( VidMode_BG0_SPR_BG1 &vid, CubeWrapper *pWrapper );
private:
    Bubble m_aBubbles[MAX_BUBBLES];
    float m_fTimeTillSpawn;
};


#endif //_BUBBLES_H
