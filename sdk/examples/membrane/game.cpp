/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "game.h"

Random Game::random;

Game::Game()
    : cube_0(0), cube_1(1), cube_2(2),
      physicsClock(60), running(true)
{}

void Game::title()
{
    /*
     * Display a title screen, from our bootstrap assets, for at least
     * 'titleTime' seconds. Load assets in the background, and keep the
     * title screen up until the loading has completed.
     *
     * We want to animate the title screen flickering, but to keep asset
     * loading fast we need to limit its frame rate. So, to keep it looking
     * interesting, we randomize the flickers.
     */

    const float titleTime = 4.0f;

    ScopedAssetLoader loader;
    AssetConfiguration<1> config;
    config.append(MainSlot, GameAssets);
    loader.start(config);

    for (CubeID i = 0; i < NUM_CUBES; ++i) {
        getGameCube(i).vid.initMode(BG0);
    }

    Random r;
    unsigned frame = 1;
    SystemTime titleDeadline = SystemTime::now() + titleTime;
    
    while (SystemTime::now() < titleDeadline || !loader.isComplete()) {

        // Stop randomly when frame==0
        if (frame || r.chance(0.1)) {
            for (CubeID i = 0; i < NUM_CUBES; ++i)
                getGameCube(i).vid.bg0.image(vec(0,0), Title, frame);
            frame++;
            if (frame == Title.numFrames())
                frame = 0;
        }

        // Frame rate division
        for (unsigned f = 0; f < 3; f++)
            System::paint();
    }
}

void Game::init()
{
    for (unsigned i = 0; i < NUM_CUBES; i++)
        getGameCube(i).init();
        
    for (unsigned i = 0; i < NUM_PARTICLES; i++)
        particles[i].instantiate(&getGameCube(i % NUM_CUBES));

    Events::neighborAdd.set(&Game::onNeighborAdd, this);
    Events::neighborRemove.set(&Game::onNeighborRemove, this);
    Events::gameMenu.set(&Game::onRestart, this, "« Restart »");
}

void Game::cleanup()
{
    Events::neighborAdd.unset();
    Events::neighborRemove.unset();
    Events::gameMenu.unset();
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

void Game::onNeighborAdd(unsigned c0, unsigned s0, unsigned c1, unsigned s1)
{
    if (c0 >= NUM_CUBES || c1 >= NUM_CUBES)
        return;

    GameCube &gc0 = getGameCube(c0);
    GameCube &gc1 = getGameCube(c1);

    // Animate the portals opening
    gc0.getPortal(s0).setOpen(true);
    gc1.getPortal(s1).setOpen(true);

    // Send particles in both directions through the new portal
    PortalPair forward = { &gc0, &gc1, s0, s1 };
    PortalPair reverse = { &gc1, &gc0, s1, s0 };
    
    for (unsigned i = 0; i < NUM_PARTICLES; i++) {
        Particle &p = particles[i];
        
        p.portalNotify(forward);
        p.portalNotify(reverse);
    }
}

void Game::onNeighborRemove(unsigned c0, unsigned s0, unsigned c1, unsigned s1)
{
    if (c0 >= NUM_CUBES || c1 >= NUM_CUBES)
        return;

    // Animate the portals closing
    getGameCube(c0).getPortal(s0).setOpen(false);
    getGameCube(c1).getPortal(s1).setOpen(false);

    /*
     * When a cube is removed, that's when we check for matches. This lets you get a fourth
     * match if you can do it without removing a cube first.
     */
    checkMatches();
}

void Game::onRestart()
{
    running = false;
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

    while (running) {
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