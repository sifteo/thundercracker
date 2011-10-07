/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_H
#define _FRONTEND_H

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   pragma comment(lib, "opengl32.lib")
#endif

#include <SDL.h>
#ifdef __MACH__
#   include <OpenGL/gl.h>
#else
#   include <GL/gl.h>
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#   define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#include <SDL.h>

#include "system.h"


struct Point {
    Point(float _x, float _y) : x(_x), y(_y) {}
    Point() : x(0), y(0) {}
    float x, y;
};


class FrontendCube {
 public:
    void init(Cube::Hardware *hw, Point center);
    void draw();

    Point center;
    static const float SIZE = 1;

 private:
    Cube::Hardware *hw;
    GLuint texture;
};


class Frontend {
 public:
    void init(System *sys);
    void run();
    void exit();
    
 private:
    /*
     * Number of real OpenGL frames per virtual LCD frame (Assume 60Hz
     * real, 30 Hz virtual). This doesn't have any bearing on the
     * rendering frame rate of the cubes, it's just used for
     * generating vertical sync signals.
     */
    static const unsigned FRAME_HZ_DIVISOR = 2;

    void draw();

    bool onResize(int width, int height);
    void onKeyDown(SDL_KeyboardEvent &evt);
    void onMouseUpdate(int x, int y, int buttons);

    System *sys;
    SDL_Surface *surface;
    unsigned frameCount;
    FrontendCube cubes[System::MAX_CUBES];
};


#endif
