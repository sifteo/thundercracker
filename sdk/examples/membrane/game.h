/*
 * Sifteo SDK Example.
 */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

#define NUM_CUBES     3
#define NUM_PARTICLES 6

extern AssetSlot MainSlot;


class Portal {
public:
    Portal(int side);

    void setOpen(bool open);
    void animate();
    void draw(VideoBuffer &vid);
    
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
    CubeHilighter(VideoBuffer &vid);
    
    void init();
    void animate(float timeStep);
    void draw();

    bool doHilight(Int2 requestedPos);
    
private:
    VideoBuffer &vid;
    TimeTicker ticker;
    int counter;
    Int2 pos;
};
    
    
class GameCube {
public:
    GameCube(CubeID id);

    void init();
    
    void animate(float timeStep);
    void draw();
    
    void placeMarker(int id);
    bool reportMatches(unsigned bits);

    Float2 velocityFromTilt();
    
    VideoBuffer vid;
    CubeHilighter hilighter;
    
    Portal portal_e, portal_w, portal_f, portal_a;
    Portal &getPortal(unsigned i) {
        switch (i) {
        default:
        case TOP:     return portal_e;
        case LEFT:    return portal_w;
        case BOTTOM:  return portal_f;
        case RIGHT:   return portal_a;
        };
    }
    
private:
    TimeTicker portalTicker;
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
        EARTH = TOP,
        WATER = LEFT,
        FIRE  = BOTTOM,
        AIR   = RIGHT,
        
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

    TimeTicker ticker;    
    Flavor flavor;
    unsigned animIndex;
    SystemTime stateDeadline;
    
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
    void cleanup();

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
    TimeTicker physicsClock;
    bool running;

    void checkMatches();

    // Event handlers
    void onNeighborAdd(unsigned c0, unsigned s0, unsigned c1, unsigned s1);
    void onNeighborRemove(unsigned c0, unsigned s0, unsigned c1, unsigned s1);
    void onRestart();
};

#endif
