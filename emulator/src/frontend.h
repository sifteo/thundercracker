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

#include <Box2D/Box2D.h>

#include "system.h"


class FrontendCube {
 public:
    void init(Cube::Hardware *hw, b2World &world, float x, float y);
    void initGL();

    void animate();
    void draw();
    void setPhysicalAcceleration(b2Vec2 g);
    void setTiltTarget(b2Vec2 angles);

    b2Body *body;
    static FrontendCube *fromBody(b2Body *body);

    /*
     * Size of the cube, in Box2D "meters". Our rendering parameters
     * are scaled relative to this size, so its affect is really
     * mostly about the units we use for the physics sim.
     */
    static const float SIZE = 0.5;

    /*
     * Size of the portion of the cube we're calling the "center". Has
     * some UI implications, like dragging on the center vs. a corner.
     * This is the radius of a circle, at the cube's center of mass.
     *
     * This is in cube-sized units (will be multiplied with SIZE to get world coordinates).
     */
    static const float CENTER_SIZE = 0.75;

    /*
     * Size of the LCD, relative to the size of the whole cube.
     */
    static const float LCD_SIZE = 0.8;

 private:
    static const GLchar *srcLcdVP[];
    static const GLchar *srcLcdFP[]; 

    void updateAccelerometer();

    b2Vec2 physicalAcceleration;
    b2Vec2 tiltTarget;
    b2Vec2 tiltVector;
    b2Mat33 modelMatrix;

    Cube::Hardware *hw;
    GLuint texture;
    GLuint lcdProgram;
    GLuint lcdFP;
    GLuint lcdVP;
};


class Frontend {
 public:
    Frontend();

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

    void animate();
    void draw();
    bool onResize(int width, int height);
    void onKeyDown(SDL_KeyboardEvent &evt);
    void onMouseDown(int buttons);
    void onMouseUp(int buttons);

    void newStaticBox(float x, float y, float hw, float hh);

    float zoomedViewExtent();
    float normalViewExtent();
    float targetViewExtent();
    b2Vec2 targetViewCenter();
    b2Vec2 mouseVec(float viewExtent);

    System *sys;
    SDL_Surface *surface;
    unsigned frameCount;
    FrontendCube cubes[System::MAX_CUBES];

    bool toggleZoom;

    int viewportWidth, viewportHeight;
    int mouseX, mouseY;

    float viewExtent;
    b2Vec2 viewCenter;

    b2World world;

    b2Body *mouseBody;
    b2Body *mousePickedBody;
    b2RevoluteJoint *mouseJoint;
    bool mouseIsAligning;
    bool mouseIsPulling;
    bool mouseIsTilting;
};


#endif
