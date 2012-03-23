/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_MOTHERSHIP_H_
#define _FRONTEND_MOTHERSHIP_H_

// Including cube's interface because she contains things like
// the fixture userdata structure, which if we were really being
// pedantic would be split out to some common header to reduce
// coupling.
// (>'')> GOOD THING WE'RE GAME DEVELOPERS AND NOT PEDANTS!
#include "frontend_cube.h" 

class FrontendMothership;

namespace MothershipConstants {

	/*
	 * Size of the mothership, int Box2D meters.
	 */
	const float SIZEX = 1.2;
	const float SIZEY = 0.5; // matches CubeConstants::SIZE

}

class FrontendMothership {
private:
	b2Body* body;
    b2Fixture *bodyFixture;
    FixtureData bodyFixtureData; // we'll need fixture data for any 
    							 // neighbor sensors if we decide to simulate
    							 // pairing in the box2D world :P

public:
	FrontendMothership();

	void init(b2World &world, float x, float y);
	void exit();

	void animate();
	void draw(GLRenderer &r);

	b2Body* getBody() { return body; }

};

#endif
