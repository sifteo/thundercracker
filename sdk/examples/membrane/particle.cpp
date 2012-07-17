/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "game.h"

 
Particle::Particle()
    : state(S_NORMAL), onCube(0), animIndex(0)
{}
 
void Particle::animate(float dt)
{
    // Sprite animation
    animIndex += ticker.tick(dt);
}

void Particle::applyPendingMove()
{
    if (state == S_MOVE_PENDING) {
        Portal &p0 = pendingMove.cube0->getPortal(pendingMove.side0);
        Portal &p1 = pendingMove.cube1->getPortal(pendingMove.side1);
        
        pos = p1.getTarget();
        velocity = -p0.rotateTo(p1, velocity);
        
        flavor.replaceElement(pendingMove.side0, pendingMove.side1);
        
        onCube = pendingMove.cube1;
        state = S_MOVE_FINISHING;
    }
}

void Particle::markForDestruction() {
    state = S_DESTROY_PENDING;
    stateDeadline = SystemTime::now() + 0.75;
}
  
void Particle::doPhysics(float dt)
{
    if (state == S_DESTROY_PENDING && stateDeadline.inPast())
        return;

    Float2 cv = LCD_center - pos;
    const float radius = 35.0f;
    
    if (state == S_MOVE_PENDING) {
        /*
         * If a move is pending, suck this particle into the correct portal.
         * Apply a strong force toward the portal, and rapidly damp any
         * existing velocity.
         *
         * If we exit the screen in this mode, the move is applied.
         * The width we use here can account for the actual screen size,
         * plus any padding that represents the area between cubes.
         */
        
        Portal &p = pendingMove.cube0->getPortal(pendingMove.side0);
        Float2 pv = p.getTarget() - pos;
        velocity = velocity * 0.8f + pv * (60.0f * dt);
        
        if (p.distanceFrom(pos) < 0.0f)
            applyPendingMove();
    
    } else if (cv.len2() > radius*radius) {
        /*
         * Too far from the center? Pull toward the middle,
         * and add a little damping.
         */
        velocity = velocity * 0.9f + cv * (15.0f * dt); 
    
    } else {
        // Respond to tilt
        if (onCube)
            velocity = velocity + onCube->velocityFromTilt();
    }
    
    // Integrate velocity
    pos += velocity * dt;
}

void Particle::doPairPhysics(Particle &other, float dt)
{
    if (onCube == other.onCube && state == S_NORMAL && other.state == S_NORMAL) {
        Float2 v = other.pos - pos;
        
        if (v.x == 0 && v.y == 0) {
            // Random nudge if two particles have the exact same location
            v.x += Game::random.randint(-1, 1);
            v.y += Game::random.randint(-1, 1);
        }
            
        Float2 force;
        float dist2 = v.len2();
        const float repelDist = 15.0f;
        
        if (dist2 > repelDist * repelDist) {
            // Far away? Gravity applies, the objects attract.
            // Rotate it a bit so they orbit.
            force = (v * (8.0f * dt)).rotate(0.24f);
            
        } else {
            // Too close? Some sort of very strong nuclear repulsion.
            force = v * (-300.0f * dt);
        }
        
        velocity = velocity + force;
        other.velocity = other.velocity - force;
    }
}

void Particle::draw(GameCube *gc, int spriteId)
{
    const auto &sprite = gc->vid.sprites[spriteId];
    const PinnedAssetImage *asset;
    unsigned frame;

    if (gc != onCube) {
        sprite.hide();
        return;
    }

    // Destruction    
    if (state == S_DESTROY_PENDING && stateDeadline.inPast() &&
        gc->hilighter.doHilight(pos.round())) {
        
        stateDeadline = SystemTime::now() + Game::random.uniform(0.5, 3.0);
        state = S_RESPAWN_PENDING;
    }
    if (state == S_RESPAWN_PENDING) {
        sprite.hide();
        if (stateDeadline.inPast())
            instantiate(gc);
        return;
    }
    
    // Normally we just wiggle a bit
    static const uint8_t frames[] = { 0, 1, 2, 3, 2, 1 };
    asset = &flavor.getAsset();
    animIndex = animIndex % arraysize(frames);
    frame = frames[animIndex] + flavor.getFrameBase();

    // Warp animations
    
    if (state == S_MOVE_PENDING || state == S_MOVE_FINISHING) {
        Portal *p;
        
        if (state == S_MOVE_PENDING)
            p = &pendingMove.cube0->getPortal(pendingMove.side0);
        else
            p = &pendingMove.cube1->getPortal(pendingMove.side1);
        
        float dist = p->distanceFrom(pos);
        int f = 6 - dist * (6.0f / 35.0f);

        if (f > 5) {
            sprite.hide();
            return;
        }
        
        if (f >= 0) {
            frame = f + flavor.getWarpFrameBase();
            asset = &flavor.getWarpAsset();
            
        } else if (state == S_MOVE_FINISHING) {
            // Done with warp-in animation
            state = S_NORMAL;
        }
    }
    
    // Drawing
    sprite.setImage(*asset, frame);
    sprite.move(pos - asset->pixelExtent());
}

void Particle::instantiate(GameCube *gc)
{
    onCube = gc;
    state = S_NORMAL;
    
    // Random particle flavor
    flavor.randomize();
    
    // Random animation speed
    ticker.setRate(Game::random.randint(5, 60));
    
    // Put it nearish the center of the screen
    int radius = 20;    
    pos.x = Game::random.randint(LCD_center.x - radius,
                                 LCD_center.x + radius);
    pos.y = Game::random.randint(LCD_center.y - radius,
                                 LCD_center.y + radius);
                
    // Random velocity
    float angle = Game::random.uniform(0, 2 * M_PI);
    float speed = Game::random.uniform(10, 50);
    velocity.setPolar(angle, speed);
}

void Particle::portalNotify(const PortalPair &pair)
{
    if (state != S_MOVE_PENDING) {
        if (pair.cube0 == onCube && flavor.hasElement(pair.side0)) {
            pendingMove = pair;
            state = S_MOVE_PENDING;
            
        } else if (pair.cube1 == onCube && flavor.hasElement(pair.side1)) {
            // Always store in from->to order.
            pendingMove.cube0 = pair.cube1;
            pendingMove.cube1 = pair.cube0;
            pendingMove.side0 = pair.side1;
            pendingMove.side1 = pair.side0;
            state = S_MOVE_PENDING;
        }
    }
}
