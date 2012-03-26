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

namespace MothershipConstants {

	/*
	 * Size of the mothership, int Box2D meters.
	 */
	const float SIZEX = 0.75;
	const float SIZEY = 0.5; // matches CubeConstants::SIZE

}

class FrontendMothership {
private:
	unsigned id; // we'll' want multiple motherships for things like pairing tests
	b2Body* body;
    b2Fixture *bodyFixture;
    FixtureData bodyFixtureData; 

    // TODO
    // FixtureData neighborFixtureData[2]; 
    // MotherShip::Hardware *hw;

public:
	FrontendMothership();

	void init(unsigned id, b2World &world, float x, float y);
	void exit();

	void animate();
	void draw(GLRenderer &r);

	unsigned getId() const { return id; }
	b2Body* getBody() { return body; }
    bool isInitialized() const { return body != 0; }

	void setResetPressed(bool isDown);

};

#endif
