/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"

FrontendMothership::FrontendMothership()
	: body(0) {}

void FrontendMothership::init(b2World& world, float x, float y) {
	
	//------------------------------------------------------------------------------
	// initialize body
	//------------------------------------------------------------------------------

	b2BodyDef bodyDef;
	bodyDef.type = b2_kinematicBody;
	bodyDef.position.Set(x,y);
	body = world.CreateBody(&bodyDef);

	b2PolygonShape box;
	const b2Vec2 boxSize = 0.96f * b2Vec2(
		MothershipConstants::SIZEX,
		MothershipConstants::SIZEY
	); // 96% magic number yanked from frontend_cube.cpp
	box.SetAsBox(boxSize.x, boxSize.y);

	bodyFixtureData.type = FixtureData::T_MOTHERSHIP;
	bodyFixtureData.ptr.mothership = this;	

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.8f;
    fixtureDef.userData = &bodyFixtureData;
    bodyFixture = body->CreateFixture(&fixtureDef);


}

void FrontendMothership::exit() {
	if (body) {
		body->GetWorld()->DestroyBody(body);
		body = 0;
	}
}

void FrontendMothership::animate() {
	// NOOP
	// Gift Idea: Hurnold Fobcakes peeks out from behind 
	// the mothership when you press the reset button.
}

void FrontendMothership::draw(GLRenderer &r) {

}

