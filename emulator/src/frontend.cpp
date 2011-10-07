/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"


void Frontend::init(System *_sys)
{
    sys = _sys;
    frameCount = 0;

    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].init(&sys->cubes[i],
                      Point( (sys->opt_numCubes * -0.5 + i) 
                             * FrontendCube::SIZE * 1.5, 0 ));

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("Thundercracker", NULL);
}

void Frontend::run()
{
    if (!onResize(600, 400))
        return;

    while (1) {
        SDL_Event event;
    
        // Drain the GUI event queue
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                
            case SDL_QUIT:
                return;

            case SDL_VIDEORESIZE:
                if (!onResize(event.resize.w, event.resize.h))
                    return;
                break;

            case SDL_KEYDOWN:
                onKeyDown(event.key);
                break;

            case SDL_MOUSEMOTION:
                onMouseUpdate(event.motion.x, event.motion.y, event.motion.state);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                onMouseUpdate(event.button.x, event.button.y, event.button.state);
                break;
            
            }
        }

        if (sys->time.isPaused())
            SDL_Delay(50);
        
        if (!(frameCount % FRAME_HZ_DIVISOR)) {
            for (unsigned i = 0; i < sys->opt_numCubes; i++)
                sys->cubes[i].lcd.pulseTE();
        }

        draw();
        frameCount++;
    }
}

void Frontend::exit()
{             
    SDL_Quit();
}

bool Frontend::onResize(int width, int height)
{
    surface = SDL_SetVideoMode(width, height, 0, SDL_OPENGL);
    if (surface == NULL) {
        fprintf(stderr, "Error creating SDL surface!\n");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    return true;
}

void Frontend::onKeyDown(SDL_KeyboardEvent &evt)
{
}

void Frontend::onMouseUpdate(int x, int y, int buttons)
{
}

void Frontend::draw()
{
    // Background
    glClearColor(0, 0, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // All cubes
    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].draw();

    SDL_GL_SwapBuffers();
}

void FrontendCube::init(Cube::Hardware *_hw, Point _center)
{
    hw = _hw;
    center = _center;
    texture = -1;
}

void FrontendCube::draw()
{
    if (texture < 0) {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, 3, Cube::LCD::WIDTH, Cube::LCD::HEIGHT,
                     0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    }

    if (hw->lcd.isVisible()) {
        // LCD on, update texture

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        Cube::LCD::WIDTH, Cube::LCD::HEIGHT,
                        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, hw->lcd.fb_mem);   
    } else {
        // LCD off, show white screen

        glDisable(GL_TEXTURE_2D);
    }

    static const GLfloat vertexArray[] = {
        0, 1,  -1,-1, 0,
        1, 1,   1,-1, 0,
        0, 0,  -1, 1, 0,
        1, 0,   1, 1, 0,
    };

    glInterleavedArrays(GL_T2F_V3F, 0, vertexArray);

    glPushMatrix();
    glTranslatef(center.x, center.y, 0);
    glScalef(SIZE, SIZE, SIZE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glPopMatrix();
}
