/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_OVERLAY_H
#define _FRONTEND_OVERLAY_H

#include "gl_renderer.h"
#include "system.h"

#include <Box2D/Box2D.h>
#include <string>


struct Color {
    Color() {}
    
    Color(float r, float g, float b, float a=1.0f) {
        set(r, g, b, a);
    }
    
    void set(float r, float g, float b, float a=1.0f) {
        v[0] = r;
        v[1] = g;
        v[2] = b;
        v[3] = a;
    }
    
    float v[4];
};    


class FrontendOverlay {
public:
    FrontendOverlay();

    void init(GLRenderer *renderer, System *sys);

    void draw();
    void drawCube(FrontendCube *fe, unsigned x, unsigned y);
    
    void postMessage(std::string msg);
    
private:
    static const unsigned margin = 5;
    static const unsigned lineSpacing = 20;
    
    void moveTo(unsigned _x, unsigned _y) {
        x = _x;
        y = _y;
    }
    
    void text(const Color &c, const char *msg, float align=0.0f);
    
    GLRenderer *renderer;
    System *sys;
    
    std::string message;
    unsigned messageTimer;
    
    ElapsedTime timer;
    char realTimeMessage[64];
    Color realTimeColor;
    
    unsigned x, y;
    
    struct {
        char fps[16];
        EventRateProbe lcd_wr;
    } cubes[System::MAX_CUBES];
};


#endif
