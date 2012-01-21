/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

#define NUM_CUBES     3
#define NUM_PARTICLES 6

#define INVALID_ID  (-1)


int randint(int min, int max);


struct Vec2F {
    float x, y;
    
    Vec2 round();
    
    Vec2F operator -(const Vec2F b) const {
        Vec2F c = { x - b.x, y - b.y };
        return c;
    }

    Vec2F operator +(const Vec2F b) const {
        Vec2F c = { x + b.x, y + b.y };
        return c;
    }

    Vec2F operator *(float b) const {
        Vec2F c = { x * b, y * b };
        return c;
    }
    
    Vec2F operator -() const {
        Vec2F c = { -x, -y };
        return c;
    }

    Vec2F rotate(float r) const {
        float s = sinf(r);
        float c = cosf(r);
        Vec2F v = { x*c - y*s, x*s + y*c };
        return v;
    }
            
    float dist2() const {
        return x*x + y*y;
    }
};


class Ticker {
public:
    Ticker();
    Ticker(float hz);

    void setRate(float hz);    
    int tick(float dt);
    
    float getPeriod() {
        return period;
    }
    
private:
    float period, accum;
};


class Sprites {
public:
    Sprites(VideoBuffer &vbuf)
        : vbuf(vbuf) {}
        
    void init();    
    
    void move(int id, Vec2 pos);
    void set(int id, const PinnedAssetImage &image, int frame=0);
    void hide(int id);
        
    void resizeSprite(int id, int w, int h);
    void setSpriteImage(int id, int tile);    
 
private:
    VideoBuffer &vbuf;
};


class Portal {
public:
    Portal(int side);

    void setOpen(bool open);
    void animate();
    void draw(Cube &cube);
    
    Vec2F getTarget() const;

    float distanceFrom(Vec2F coord);
    Vec2F rotateTo(const Portal &dest, Vec2F coord);
    
private:
    enum {
        S_CLOSED,
        S_CLOSING,
        S_OPEN,
        S_OPENING,
    } state;
    
    uint8_t side;
    uint8_t frame;
};


class CubeHilighter {
public:
    CubeHilighter(Cube &cube);
    
    void init();
    void animate(float timeStep);
    void draw();

    bool doHilight(Vec2 requestedPos);
    
private:
    Cube &cube;
    Ticker ticker;
    int counter;
    Vec2 pos;
};
    
    
class GameCube {
public:
    GameCube(int id);

    void init();
    
    void animate(float timeStep);
    void draw();
    
    void placeMarker(int id);
    bool reportMatches(unsigned bits);
    
    void setNeighbor(int side, int id);
    int getNeighbor(int side);
    
    Vec2F velocityFromTilt();
    
    Cube cube;
    CubeHilighter hilighter;
    
    Portal portal_e, portal_w, portal_f, portal_a;
    Portal &getPortal(unsigned i) {
        switch (i) {
        default:
        case SIDE_TOP:     return portal_e;
        case SIDE_LEFT:    return portal_w;
        case SIDE_BOTTOM:  return portal_f;
        case SIDE_RIGHT:   return portal_a;
        };
    }
    
private:
    int neighbors[NUM_SIDES];
    Ticker portalTicker;
    unsigned numMarkers;
};


class Flavor {
public:
    void randomize();
    
    const PinnedAssetImage &getAsset();
    unsigned getFrameBase();

    const PinnedAssetImage &getWarpAsset();
    unsigned getWarpFrameBase();
    
    bool hasElement(unsigned e) const;
    void replaceElement(unsigned from, unsigned to);

    unsigned getMatchBit() const;

private:
    enum Element {
        EARTH = SIDE_TOP,
        WATER = SIDE_LEFT,
        FIRE = SIDE_BOTTOM,
        AIR = SIDE_RIGHT,
        
        FIRST_ELEMENT = EARTH,
        LAST_ELEMENT = AIR,
    };
        
    union {
        uint32_t elementWord;
        uint8_t elements[2];
    };
};


struct PortalPair {
    GameCube *cube0, *cube1;
    unsigned side0, side1;
};


class Particle {
public:
    Particle();

    void animate(float dt);
    void doPhysics(float dt);
    void doPairPhysics(Particle &other, float dt);
    void draw(GameCube *gc, int spriteId);
    
    void instantiate(GameCube *gc);
    void portalNotify(const PortalPair &pair);
    
    bool isOnCube(GameCube *cube) {
        return onCube == cube;
    }
    
    const Flavor &getFlavor() const {
        return flavor;
    }
    
    void markForDestruction();
    
    bool isMatchable() {
        return state != S_DESTROY_PENDING && state != S_RESPAWN_PENDING;
    }
    
private:
    enum {
        S_NORMAL,
        S_MOVE_PENDING,
        S_MOVE_FINISHING,
        S_DESTROY_PENDING,
        S_RESPAWN_PENDING,
    } state;

    Vec2F pos, velocity;
    GameCube *onCube;

    Ticker ticker;    
    Flavor flavor;
    unsigned animIndex;
    float stateDeadline;
    
    PortalPair pendingMove;
    
    void applyPendingMove();
};


class Game {
public:
    Game();

    void loadAssets();
    void title();
    void init();
    void run();

    void animate(float dt);
    void doPhysics(float dt);
    void draw();    
    
private:
    GameCube cube_0, cube_1, cube_2;
    GameCube &getGameCube(unsigned i) {
        switch (i) {
        default:
        case 0: return cube_0;
        case 1: return cube_1;
        case 2: return cube_2;
        };
    }
    
    Particle particles[NUM_PARTICLES];
    Ticker physicsClock;
    
    void checkNeighbors();
    void checkMatches();
};

#endif
