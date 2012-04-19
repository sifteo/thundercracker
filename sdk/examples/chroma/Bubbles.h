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
class GridSlot;

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
    void Draw( VideoBuffer &vid, int index, CubeWrapper *pWrapper ) __attribute__ ((noinline));
    inline bool isAlive() const { return m_fTimeAlive >= 0.0f; }

private:
    //check if we're bumping against a chromit
    bool CheckBump( Float2 diff, GridSlot *pSlot ) __attribute__ ((noinline));

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

    BubbleSpawner( VideoBuffer &vid );
    void Reset( VideoBuffer &vid );
    void Update( float dt, const Float2 &tilt );
    void Draw( VideoBuffer &vid, CubeWrapper *pWrapper ) __attribute__ ((noinline));
    inline bool isActive() { return m_bActive; }
private:
    Bubble m_aBubbles[MAX_BUBBLES];
    float m_fTimeTillSpawn;
    bool m_bActive;
};


#endif //_BUBBLES_H
