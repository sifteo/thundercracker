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

}

class FrontendMC {
private:
    b2Body* body;
    b2Fixture *bodyFixture;
    FixtureData bodyFixtureData; 

public:
    FrontendMC();

    void init(b2World &world, float x, float y);
    void exit();

    void draw(GLRenderer &r);

    b2Body* getBody() { return body; }
    bool isInitialized() const { return body != 0; }

    void setButtonPressed(bool isDown);
};

#endif
