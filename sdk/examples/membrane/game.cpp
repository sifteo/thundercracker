/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <sifteo.h>
#include "game.h"

Random Game::random;

Game::Game()
    : cube_0(0), cube_1(1), cube_2(2),
      physicsClock(60)
{}

void Game::title()
{
    const float titleTime = 4.0f;

    for (unsigned i = 0; i < NUM_CUBES; i++) {
        VidMode_BG0 vid(getGameCube(i).cube.vbuf);
        vid.init();
    }

    unsigned frame = 0;
    SystemTime titleDeadline = SystemTime::now() + titleTime;
    
    while (SystemTime::now() < titleDeadline) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0 vid(getGameCube(i).cube.vbuf);
            vid.BG0_drawAsset(vec(0,0), Title, frame % Title.frames);
        }
        System::paint();
        frame++;
    }
}

void Game::init()
{
    for (unsigned i = 0; i < NUM_CUBES; i++)
        getGameCube(i).init();
        
    for (unsigned i = 0; i < NUM_PARTICLES; i++)
        particles[i].instantiate(&getGameCube(i % NUM_CUBES));

    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborAdd, (void*) this);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighborRemove, (void*) this);
}

void Game::animate(float dt)
{
    for (unsigned i = 0; i < NUM_PARTICLES; i++)
        particles[i].animate(dt);

    for (unsigned i = 0; i < NUM_CUBES; i++)
        getGameCube(i).animate(dt);
}
        
void Game::doPhysics(float dt)
{
    for (unsigned i = 0; i < NUM_PARTICLES; i++) {
        particles[i].doPhysics(dt);

        for (unsigned j = i+i; j < NUM_PARTICLES; j++)
            particles[i].doPairPhysics(particles[j], dt);
    }
}

void Game::onNeighborAdd(Game *self,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
	GameCube &gc0 = self->getGameCube(c0);
	GameCube &gc1 = self->getGameCube(c1);

	// Animate the portals opening
	gc0.getPortal(s0).setOpen(true);
	gc1.getPortal(s1).setOpen(true);

	// Send particles in both directions through the new portal
	PortalPair forward = { &gc0, &gc1, s0, s1 };
	PortalPair reverse = { &gc1, &gc0, s1, s0 };
	
	for (unsigned i = 0; i < NUM_PARTICLES; i++) {
		Particle &p = self->particles[i];
		
		p.portalNotify(forward);
		p.portalNotify(reverse);
	}
}

void Game::onNeighborRemove(Game *self,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
	// Animate the portals closing
	self->getGameCube(c0).getPortal(s0).setOpen(false);
	self->getGameCube(c1).getPortal(s1).setOpen(false);

	/*
	 * When a cube is removed, that's when we check for matches. This lets you get a fourth
	 * match if you can do it without removing a cube first.
	 */
	self->checkMatches();
}

void Game::draw()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        GameCube &gc = getGameCube(i);
        
        gc.draw();

        for (unsigned i = 0; i < NUM_PARTICLES; i++)
            particles[i].draw(&gc, i);
    }
}

void Game::run()
{
    TimeStep ts;

    while (1) {
        ts.next();
        
        // Real-time for animations
        animate(ts.delta());
        
        // Fixed timesteps for physics
        for (int i = physicsClock.tick(ts.delta()); i; i--)
            doPhysics(physicsClock.getPeriod());

        // Put stuff on the screen!
        draw();
        System::paint();
    }
}

void Game::checkMatches()
{
    /*
     * If a cube has at least three different colors of single-color
     * particles, that cube has a match; all particles on that cube
     * are dissolved.
     */
    
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        GameCube &gc = getGameCube(i);
        unsigned matches = 0;
        
        for (unsigned j = 0; j < NUM_PARTICLES; j++) {
            Particle &p = particles[j];
            const Flavor &flavor = p.getFlavor();
            
            if (p.isMatchable() && p.isOnCube(&gc))
                matches |= flavor.getMatchBit();
        }

        if (gc.reportMatches(matches)) {
            // There was a match; mark these particles for destruction
            
            for (unsigned j = 0; j < NUM_PARTICLES; j++) {
                Particle &p = particles[j];
                if (p.isOnCube(&gc))
                    p.markForDestruction();
            }
        }        
    }
}