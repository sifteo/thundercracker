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
#   include <OpenGL/glext.h>
#else
#   define GL_GLEXT_PROTOTYPES
#   include <GL/gl.h>
#   include <GL/glext.h>
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#   define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#include <Box2D/Box2D.h>
#include "system.h"


class FrontendCube;


class AccelerationProbe {
 public:
    AccelerationProbe();

    b2Vec3 measure(b2Body *body, float unitsToGs);

 private:
    static const unsigned filterWidth = 8;
    unsigned head;
    b2Vec2 velocitySamples[filterWidth];
};


struct FixtureData {
    enum Type {
        T_CUBE = 0,
        T_NEIGHBOR,
    };

    Type type;
    FrontendCube *cube;
    Cube::Neighbors::Side side;
};


class FrontendCube {
 public:
    void init(Cube::Hardware *hw, b2World &world, float x, float y);
    void initGL();

    void animate();
    void draw();

    void setTiltTarget(b2Vec2 angles);
    void updateNeighbor(bool touching, unsigned mySide,
                        unsigned otherSide, unsigned otherCube);

    b2Body *body;

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
    static const float CENTER_SIZE = 0.6;

    /*
     * Size of the LCD, relative to the size of the whole cube.
     */
    static const float LCD_SIZE = 0.75;

    /*
     * Height of the cube, relative to SIZE
     */
    static const float HEIGHT = 0.4;

    /*
     * The sensitive region for this cube's neighbor transceivers.
     *
     * Both relative to SIZE. These regions are modeled as circles,
     * and whenever two circles *touch* the sensor is active. This is
     * in contrast to the usual definition of sensor range, in which
     * the center point of one sensor must be within the range of the
     * other sensor. To compensate, this radius should be 1/2 the
     * sensor's actual range.
     */
    static const float NEIGHBOR_CENTER = 0.9;
    static const float NEIGHBOR_RADIUS = 0.15;

 private:
    void initBody(b2World &world, float x, float y);
    void initNeighbor(Cube::Neighbors::Side side, float x, float y);

    FixtureData bodyFixtureData;
    FixtureData neighborFixtureData[Cube::Neighbors::NUM_SIDES];

    static const GLchar *srcLcdVP[];
    static const GLchar *srcLcdFP[]; 
    
    AccelerationProbe accel;
    b2Vec2 tiltTarget;
    b2Vec2 tiltVector;
    b2Mat33 modelMatrix;

    Cube::Hardware *hw;
    GLuint texture;
    GLhandleARB lcdProgram;
    GLhandleARB lcdFP;
    GLhandleARB lcdVP;
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

    class MousePicker : public b2QueryCallback {
    public:
        void test(b2World &world, b2Vec2 point);
        bool ReportFixture(b2Fixture *fixture);
        b2Vec2 mPoint;
        FrontendCube *mCube;
    };

    class ContactListener : public b2ContactListener {
    public:
        ContactListener(Frontend &fe) : frontend(fe) {};
        void BeginContact(b2Contact *contact);
        void EndContact(b2Contact *contact);
    private:
        Frontend &frontend;
        void updateSensors(b2Contact *contact);
    };

    void animate();
    void draw();

    bool onResize(int width, int height);
    void onKeyDown(SDL_KeyboardEvent &evt);
    void onMouseDown(int button);
    void onMouseUp(int button);

    void newStaticBox(float x, float y, float hw, float hh);
    unsigned cubeID(FrontendCube *cube);

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
    b2RevoluteJoint *mouseJoint;
    bool mouseIsAligning;
    bool mouseIsPulling;
    bool mouseIsTilting;

    MousePicker mousePicker;
    ContactListener contactListener;
};

#endif
