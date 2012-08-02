/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_MC_H_
#define _FRONTEND_MC_H_

#include "frontend_fixture.h"
#include "ledsequencer.h"

class GLRenderer;

namespace MCConstants {

    /*
     * Size of the Base, int Box2D meters.
     */
    const float SIZEX = 1.02;
    const float SIZEY = 0.5;

    /*
     * Radius of the portion of the cube that counts as the home button.
     * In Box2D meters.
     */
    const float CENTER_SIZE = 0.18;

    /*
     * The sensitive region for this cube's neighbor transceivers,
     * in Box2D meters.
     *
     * These regions are modeled as circles, and whenever two circles
     * *touch* the sensor is active. This is in contrast to the usual
     * definition of sensor range, in which the center point of one
     * sensor must be within the range of the other sensor. To
     * compensate, this radius should be 1/2 the sensor's actual range.
     */
    const float NEIGHBOR_X = SIZEX * 0.9f;
    const float NEIGHBOR_RADIUS = SIZEY * 0.15f;
}

class FrontendMC {
private:
    b2Body* body;
    b2Fixture *bodyFixture;
    FixtureData bodyFixtureData; 
    FixtureData neighborFixtureData[2];

    double nextLEDTick;
    float ledColor[3];

public:
    FrontendMC();

    void init(b2World &world, float x, float y);
    void exit();

    void animate();
    void draw(GLRenderer &r);

    b2Body* getBody() { return body; }
    bool isInitialized() const { return body != 0; }

    // Hook for LED::set()
    static LEDSequencer led;

private:
    void initNeighbor(unsigned id, float x);

    static void accumulateLEDColor(LEDSequencer::LEDState state, float color[3]);
};

#endif
