/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_H
#define _FRONTEND_H

#include <SDL.h>
#include <Box2D/Box2D.h>
#include <string>
#include "system.h"
#include "gl_renderer.h"


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
    void init(unsigned id, Cube::Hardware *hw, b2World &world, float x, float y);

    void animate();
    void draw(GLRenderer &r);

    void setTiltTarget(b2Vec2 angles);
    void updateNeighbor(bool touching, unsigned mySide,
                        unsigned otherSide, unsigned otherCube);

    void setHoverTarget(float h);
    
    void setTouch(float amount) {
        hw->setTouch(amount);
    }
    
    bool isHovering() {
        return hoverTarget > HEIGHT;
    }

    b2Body *body;

    /*
     * Size of the cube, in Box2D "meters". Our rendering parameters
     * are scaled relative to this size, so its affect is really
     * mostly about the units we use for the physics sim.
     */
    static const float SIZE = 0.5;

    /*
     * Height of the cube, relative to its half-width
     */
    static const float HEIGHT = 0.6;

    /*
     * Size of the portion of the cube we're calling the "center". Has
     * some UI implications, like dragging on the center vs. a corner.
     * This is the radius of a circle, at the cube's center of mass.
     *
     * This is in cube-sized units (will be multiplied with SIZE to get world coordinates).
     */
    static const float CENTER_SIZE = 0.6;

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
    
    /*
     * We use different hover heights to signify various UI actions.
     * A very slight hover means the cube is being manipulated. A more
     * extreme hover means it's actually being picked up high enough
     * to not touch non-hovering cubes.
     */
    static const float HOVER_NONE = 0.0;
    static const float HOVER_SLIGHT = 0.3;
    static const float HOVER_FULL = 2.0;

 private:
    void initBody(b2World &world, float x, float y);
    void initNeighbor(Cube::Neighbors::Side side, float x, float y);

    b2Fixture *bodyFixture;
    FixtureData bodyFixtureData;
    FixtureData neighborFixtureData[Cube::Neighbors::NUM_SIDES];

    AccelerationProbe accel;
    b2Vec2 tiltTarget;
    b2Vec2 tiltVector;
    float hover, hoverTarget;
    b2Mat33 modelMatrix;

    unsigned id;
    Cube::Hardware *hw;
};


class Frontend {
 public:
    Frontend();

    void init(System *sys);
    void run();
    void exit();
    
 private:
    /*
     * Number of real frames per virtual LCD frame (Assume 60Hz
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

    bool onResize(int width, int height, bool fullscreen=false);
    void onKeyDown(SDL_KeyboardEvent &evt);
    void onMouseDown(int button);
    void onMouseUp(int button);
    void hoverOrRotate();

    void scaleViewExtent(float ratio);
    void createWalls();
    void moveWalls(bool immediate=false);
    void pushBodyTowards(b2Body *b, b2Vec2 target, float gain);

    b2Body *newKBox(float x, float y, float hw, float hh);
    unsigned cubeID(FrontendCube *cube);

    float zoomedViewExtent();
    float targetViewExtent();
    b2Vec2 targetViewCenter();
    b2Vec2 mouseVec(float viewExtent);
	
	std::string createScreenshotName();
	
    System *sys;
    SDL_Surface *surface;
    unsigned frameCount;
    FrontendCube cubes[System::MAX_CUBES];

    bool toggleZoom;
    bool isFullscreen;
    bool isRunning;

    int viewportWidth, viewportHeight;
    int mouseX, mouseY;
    unsigned gridW, gridH;

    float viewExtent;
    float normalViewExtent;
    float maxViewExtent;
    b2Vec2 viewCenter;

    b2World world;
    b2Body *walls[4];

    b2Body *mouseBody;
    b2RevoluteJoint *mouseJoint;
    float spinTarget;
    bool mouseIsAligning;
    bool mouseIsSpinning;
    bool mouseIsPulling;
    bool mouseIsTilting;
    
    MousePicker mousePicker;
    ContactListener contactListener;

    GLRenderer renderer;
};

#endif
