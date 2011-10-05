/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   pragma comment(lib, "opengl32.lib")
#endif

#ifdef __MACH__
#   include <OpenGL/gl.h>
#else
#   include <GL/gl.h>
#endif

#include <stdint.h>
#include <SDL.h>

#include "emu8051.h"
#include "emulator.h"
#include "frontend.h"
#include "lcd.h"
#include "accel.h"
#include "flash.h"
#include "heatpalette.h"

#ifndef GL_UNSIGNED_SHORT_5_6_5
#   define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#define PROFILER_BYTES  16384
#define PROFILER_LINES  (PROFILER_BYTES / LCD_WIDTH)

#define FLASH_TILE_SIZE    8
#define FLASH_TILE_BYTES   (FLASH_TILE_SIZE * FLASH_TILE_SIZE * 2)
#define FLASH_TILE_WIDTH   192
#define FLASH_TILE_HEIGHT  86

static struct {
    SDL_Surface *surface;
    struct em8051 *cpu;
    int frame_hz_divisor;
    int profiler_div_timer;
    int running;

    // Display modes, controllable at runtime
    int scale;
    int enable_profiler;
    int enable_flash;
} frontend;


static uint64_t clamp64(uint64_t val, uint64_t min, uint64_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

static uint32_t clamp32(uint32_t val, uint32_t min, uint32_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

static void frontend_update_profiler(uint16_t *dest)
{
    static uint64_t shadow[PROFILER_BYTES];
    struct profile_data *profiler;
    uint64_t *shadow_p;
    uint64_t max_count = 0;

    // First pass, look for the maximum count
    for (shadow_p = shadow, profiler = frontend.cpu->mProfilerMem;
         shadow_p < shadow + PROFILER_BYTES; shadow_p++, profiler++) {
        uint64_t count = profiler->total_cycles - *shadow_p;
        if (count > max_count)
            max_count = count;
    }

    // Second pass, draw the map
    for (shadow_p = shadow, profiler = frontend.cpu->mProfilerMem;
         shadow_p < shadow + PROFILER_BYTES; shadow_p++, profiler++) {
        uint64_t count = profiler->total_cycles - *shadow_p;
        uint16_t pixel;

        if (count)
            pixel = heatpalette[count * (sizeof heatpalette / sizeof heatpalette[0] - 1) / max_count];
        else
            pixel = 0;

        *shadow_p = profiler->total_cycles; 
        *(dest++) = pixel;                  
    }
}

static void frontend_update_texture(void)
{
    if (frontend.enable_flash) {
        unsigned tx, ty;
        unsigned remaining = flash_size();
        uint8_t *ptr = flash_buffer();

        glEnable(GL_TEXTURE_2D);

        for (ty = 0; ty < FLASH_TILE_HEIGHT; ty++)
            for (tx = 0; tx < FLASH_TILE_WIDTH; tx++) {

                glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                tx * FLASH_TILE_SIZE, ty * FLASH_TILE_SIZE,
                                FLASH_TILE_SIZE, FLASH_TILE_SIZE,
                                GL_RGB, GL_UNSIGNED_SHORT_5_6_5, ptr);
                glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

                remaining -= FLASH_TILE_BYTES;
                ptr += FLASH_TILE_BYTES;

                if (remaining < FLASH_TILE_BYTES)
                    return;
            }

    } else {
        uint16_t *fb = lcd_framebuffer();
        GLsizei width = LCD_WIDTH;
        GLsizei height = LCD_HEIGHT;

        if (fb) {
            glEnable(GL_TEXTURE_2D);
        } else {
            glDisable(GL_TEXTURE_2D);
            return;
        }

        if (frontend.enable_profiler) {
            // Update the profiler only every N frames
            if (++frontend.profiler_div_timer >= 3) {
                frontend.profiler_div_timer = 0;
                frontend_update_profiler(fb + (LCD_WIDTH * LCD_HEIGHT));
            }

            height += PROFILER_LINES;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, fb);
    }
}

static void frontend_draw_frame(void)
{
    static const GLfloat vertexArray[] = {
        0, 1,  -1,-1, 0,
        1, 1,   1,-1, 0,
        0, 0,  -1, 1, 0,
        1, 0,   1, 1, 0,
    };

    glInterleavedArrays(GL_T2F_V3F, 0, vertexArray);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    SDL_GL_SwapBuffers();
}

static void frontend_resize_window(void)
{
    unsigned texWidth, texHeight;
    unsigned winWidth, winHeight;

    if (frontend.enable_flash) {

        texWidth = FLASH_TILE_SIZE * FLASH_TILE_WIDTH;
        texHeight = FLASH_TILE_SIZE * FLASH_TILE_HEIGHT;

        winWidth = texWidth;
        winHeight = texHeight;

    } else {
        texWidth = LCD_WIDTH;
        texHeight = LCD_HEIGHT;

        if (frontend.enable_profiler)
            texHeight += PROFILER_LINES;

        winWidth = texWidth * frontend.scale;
        winHeight = texHeight * frontend.scale;
    }
       
    frontend.surface = SDL_SetVideoMode(winWidth, winHeight, 16, SDL_OPENGL);
    if (frontend.surface == NULL) {
        printf("Error creating SDL surface!\n");
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    glViewport(0, 0, winWidth, winHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glShadeModel(GL_SMOOTH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, texWidth, texHeight, 0, GL_RGB,
                 GL_UNSIGNED_SHORT_5_6_5, NULL);

    frontend_update_texture();
    frontend_draw_frame();
}

static uint8_t frontend_scale_coord(uint32_t coord, uint16_t width)
{
    /*
     * Convert a pixel coordinate to a normalized value in the range
     * [0, 0xFF], with proper rounding and boundary condition
     * handling.
     */
    uint32_t max_coord = width - 1;
    return clamp32(0, 0xFF, (coord * 0xFF + max_coord / 2) / max_coord);
}

static void frontend_mouse_update(uint16_t x, uint16_t y, uint8_t buttons)
{
    int8_t scaled_x = frontend_scale_coord(x, frontend.scale * LCD_WIDTH) - 0x80;
    int8_t scaled_y = frontend_scale_coord(y, frontend.scale * LCD_HEIGHT) - 0x80;

    if (buttons & SDL_BUTTON_LEFT) {
        /*
         * Mouse drag: Simulate a tilt
         * XXX: Axes swapped to match the prototype hardware.
         */
        accel_set_vector(scaled_y, scaled_x);
    } else {
        // Idle: Centered
        accel_set_vector(0, 0);
    }
}

void frontend_init(struct em8051 *cpu)
{
    SDL_Init(SDL_INIT_VIDEO);
    frontend.running = 1;
    frontend.scale = 2;
    frontend.cpu = cpu;

    // XXX: Assuming 60Hz host display, 30Hz emulated rate
    frontend.frame_hz_divisor = 2;

    frontend_resize_window();
    frontend_mouse_update(0, 0, 0);
    SDL_WM_SetCaption("Thundercracker", NULL);
}

void frontend_exit(void)
{
    SDL_Quit();
}

static void frontend_keydown(SDL_KeyboardEvent *evt)
{
    switch (evt->keysym.sym) {

        // Change scale
    case 's':
        if (++frontend.scale == 6)
            frontend.scale = 1;
        frontend_resize_window();
        break;

        // Toggle profiler
    case 'p':
        frontend.enable_profiler = !frontend.enable_profiler;
        frontend_resize_window();
        break;

        // Toggle flash
    case 'f':
        frontend.enable_flash = !frontend.enable_flash;
        frontend_resize_window();
        break;

    }
}

void frontend_loop(void)
{
    while (frontend.running) {
        SDL_Event event;
    
        // Drain the GUI event queue
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                
            case SDL_QUIT:
                frontend_async_quit();
                break;

            case SDL_KEYDOWN:
                frontend_keydown(&event.key);
                break;

            case SDL_MOUSEMOTION:
                frontend_mouse_update(event.motion.x, event.motion.y, event.motion.state);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                frontend_mouse_update(event.button.x, event.button.y, event.button.state);
                break;
            
            }
        }

        if (!runmode) {
            // Paused
            SDL_Delay(50);
        }

        if (frontend.running) {
            int i;
            
            frontend_update_texture();
            lcd_te_pulse();
            
            for (i = 0; i < frontend.frame_hz_divisor; i++)
                frontend_draw_frame();
        }
    }
}

void frontend_async_quit(void)
{
    frontend.running = 0;
}
