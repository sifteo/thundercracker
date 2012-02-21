/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;
using namespace Sifteo::Math;

#define NUM_CUBES     3
#define NUM_PARTICLES 6


class Ticker {
public:
    Ticker() : accum(0) {}

    Ticker(float hz) : accum(0) {
		setRate(hz);
	}

    void setRate(float hz) {
	    period = 1.0f / hz;
	}

    int tick(float dt) {
		accum += dt;
		int frames = accum / period;
		accum -= frames * period;
		return frames;
	}
    
    float getPeriod() {
        return period;
    }
    
private:
    float period, accum;
};


class Portal {
public:
    Portal(int side);

    void setOpen(bool open);
    void animate();
    void draw(Cube &cube);
    
    Float2 getTarget() const;

    float distanceFrom(Float2 coord);
    Float2 rotateTo(const Portal &dest, const Float2 &coord);
    
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

    Float2 velocityFromTilt();
    
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

    Float2 pos, velocity;
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
    
	static Random random;

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

    void checkMatches();

    // Event handlers
    static void onNeighborAdd(Game *self,
        _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1);
    static void onNeighborRemove(Game *self,
        _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1);
};

#endif
