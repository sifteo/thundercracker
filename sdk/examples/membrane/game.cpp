/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <stdlib.h>
#include <sifteo.h>

#include "game.h"


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
    float titleDeadline = System::clock() + titleTime;
    
    while (System::clock() < titleDeadline) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0 vid(getGameCube(i).cube.vbuf);
            vid.BG0_drawAsset(Vec2(0,0), Title, frame % Title.frames);
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
}

void Game::animate(float dt)
{
    checkNeighbors();
   
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
    float lastTime = System::clock();

    while (1) {
        float now = System::clock();
        float dt = now - lastTime;
        lastTime = now;
        
        // Real-time for animations
        animate(dt);
        
        // Fixed timesteps for physics
        for (int i = physicsClock.tick(dt); i; i--)
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