/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _FRONTEND_CUBE_H
#define _FRONTEND_CUBE_H

#include <Box2D/Box2D.h>
#include "system.h"
#include "frontend_fixture.h"

class FrontendCube;


namespace CubeConstants {

    // These were static-const, but the inline initializers did not cooperate with
    // Clang, so I moved them out here.

    /*
     * Size of the cube, in Box2D "meters". Our rendering parameters
     * are scaled relative to this size, so its affect is really
     * mostly about the units we use for the physics sim.
     */
    const float SIZE = 0.5;

    /*
     * Height of the cube, relative to its half-width.
     * This MUST equal the Z coordinate of the cube face model.
     */
    const float HEIGHT = 0.908884;

    /*
     * Size of the portion of the cube we're calling the "center". Has
     * some UI implications, like dragging on the center vs. a corner.
     * This is the radius of a circle, at the cube's center of mass.
     *
     * This is in cube-sized units (will be multiplied with SIZE to get world coordinates).
     */
    const float CENTER_SIZE = 0.6;

    /*
     * Size of the portion of the cube covered by the LCD image.
     *
     * This is in cube-sized units (will be multiplied with SIZE to get world coordinates).
     */
    const float LCD_SIZE = 0.645;

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
    const float NEIGHBOR_CENTER = 0.9f;
    const float NEIGHBOR_RADIUS = 0.15f;

    /*
     * We use different hover heights to signify various UI actions.
     * A very slight hover means the cube is being manipulated. A more
     * extreme hover means it's actually being picked up high enough
     * to not touch non-hovering cubes.
     */
    const float HOVER_NONE = 0.0f;
    const float HOVER_SLIGHT = 0.3f;
    const float HOVER_FULL = 2.0f;
}

class AccelerationProbe {
 public:
    AccelerationProbe();

    b2Vec3 measure(b2Body *body, float unitsToGs);

 private:
    static const unsigned filterWidth = 8;
    unsigned head;
    b2Vec2 velocitySamples[filterWidth];
};

class FrontendCube {
 public:
    FrontendCube();

    void init(unsigned id, Cube::Hardware *hw, b2World &world, float x, float y);
    void exit();

    void animate();
    bool draw(GLRenderer &r);

    void updateNeighbor(bool touching, unsigned mySide,
                        unsigned otherSide, unsigned otherCube);

    void setTiltTarget(b2Vec2 angles);
    void setHoverTarget(float h);
    void setRotationLock(bool isRotationFixed);
    void toggleFlip();
    
    void setTouch(float amount) {
        hw->setTouch(amount);
    }
    
    bool isHovering() {
        return hoverTarget > CubeConstants::HEIGHT;
    }

    bool isFlipped() const {
        return flipped;
    }
    
    unsigned getId() const {
        return id;
    }
    
    bool isInitialized() const {
        return body != 0;
    }

    void computeAABB(b2AABB &aabb);

private:
    b2Body *body;

public:
    b2Body* getBody() { return body; }

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
    bool flipped;
    b2Mat33 modelMatrix;

    unsigned id;
    Cube::Hardware *hw;
    
    uint32_t lastLcdCookie;
};

#endif
