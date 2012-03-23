/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */


#pragma once

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

public:
	FrontendMothership();

	void init(b2World &world, float x, float y);
	void exit();

	void animate();
	void draw(GLRenderer &r);

	b2Body* getBody() { return body; }

};