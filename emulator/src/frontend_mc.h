/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc.
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

    void init(b2World& world, float x, float y, bool isRotationFixed);
    void exit();

    void animate();
    void draw(GLRenderer &r);

    b2Body* getBody() { return body; }
    bool isInitialized() const { return body != 0; }
    void setRotationLock(bool isRotationFixed);

    // Hook for LED::set()
    static LEDSequencer led;

private:
    void initNeighbor(unsigned id, float x);

    static void accumulateLEDColor(LEDSequencer::LEDState state, float color[3]);
};

#endif
