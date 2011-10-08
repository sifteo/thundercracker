/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"

/*
 * Shader for rendering the LCD screens. We could probably do something
 * more complicated with simulating the subpixels in the LCD screens, but
 * right now I just have two goals:
 *
 *   1. See each pixel distinctly at >100% zooms
 *   2. No visible aliasing artifacts
 *
 * This uses a simple algorithm that decomposes texture coordinates
 * into a fractional and whole pixel portion, and applies a smooth
 * nonlinear interpolation across pixel boundaries:
 *
 * http://www.iquilezles.org/www/articles/texture/texture.htm
 */

const GLchar *FrontendCube::srcLcdVP[] = {
    "varying vec2 imageCoord;"
    "void main() {"
        "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
        "imageCoord = gl_MultiTexCoord0.xy;"
    "}"
};

const GLchar *FrontendCube::srcLcdFP[] = {
    "varying vec2 imageCoord;"
    "uniform sampler2D image;"
    "void main() {"
        "vec2 p = imageCoord*128.0+0.5;"
        "vec2 i = floor(p);"
        "vec2 f = p-i;"
        "f = f*f*f*(f*(f*6.0-15.0)+10.0);"
        "p = i+f;"
        "p = (p-0.5)/128.0;"
        "gl_FragColor = texture2D(image, p);"
    "}"
};

void FrontendCube::init(Cube::Hardware *_hw, Point _center)
{
    hw = _hw;
    center = _center;
    texture = 0;
}

void FrontendCube::initGL()
{
    lcdVP = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(lcdVP, 1, &srcLcdVP[0], NULL);
    glCompileShader(lcdVP);

    lcdFP = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(lcdFP, 1, &srcLcdFP[0], NULL);
    glCompileShader(lcdFP);

    lcdProgram = glCreateProgram();
    glAttachShader(lcdProgram, lcdVP);
    glAttachShader(lcdProgram, lcdFP);
    glLinkProgram(lcdProgram);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, Cube::LCD::WIDTH, Cube::LCD::HEIGHT,
                 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void FrontendCube::draw()
{
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

    glUseProgram(lcdProgram);
    glUniform1i(glGetUniformLocation(lcdProgram, "image"), 0);

    glPushMatrix();
    glTranslatef(center.x, center.y, 0);
    glScalef(SIZE, SIZE, SIZE);

    glInterleavedArrays(GL_T2F_V3F, 0, vertexArray);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glPopMatrix();
}
