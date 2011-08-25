/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <SDL.h>
#include <GL/gl.h>
#include "emu8051.h"
#include "emulator.h"
#include "frontend.h"
#include "lcd.h"

#define PROFILER_BYTES  16384
#define PROFILER_LINES  (PROFILER_BYTES / LCD_WIDTH)
#define RGB565(r,g,b)   ((((r) >> 3)<<11) | (((g) >> 2)<<5) | ((b) >> 3))

static struct {
    SDL_Surface *surface;
    struct em8051 *cpu;
    int frame_hz_divisor;
    int running;
    int scale;
} frontend;


static uint8_t clamp64(uint64_t val, uint64_t min, uint64_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

static void frontend_update_profiler(uint16_t *dest)
{
    static uint64_t shadow[PROFILER_BYTES];
    uint64_t *profiler = frontend.cpu->mProfilerMem;
    uint64_t *shadow_p = shadow;

    for (shadow_p = shadow; shadow_p < shadow + PROFILER_BYTES; shadow_p++) {
	uint64_t count = *profiler - *shadow_p;
	uint64_t heat_r = count ? clamp64(count / 1, 64, 255) : 0;
	uint64_t heat_gb = count ? clamp64(count / 10, 64, 255) : 0;
	uint16_t pixel = RGB565(heat_r, heat_gb, heat_gb);

	*shadow_p = *profiler; 
	profiler++;
	*(dest++) = pixel;		    
    }
}

static void frontend_update_texture(void)
{
    uint16_t *fb = lcd_framebuffer();
    GLsizei width = LCD_WIDTH;
    GLsizei height = LCD_HEIGHT;

    if (opt_visual_profiler) {
	frontend_update_profiler(fb + (LCD_WIDTH * LCD_HEIGHT));
	height += PROFILER_LINES;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB,
		 GL_UNSIGNED_SHORT_5_6_5, fb);
}

static void frontend_draw_frame(void)
{
    int i;
    static const GLfloat vertexArray[] = {
	0, 1,  -1,-1, 0,
	1, 1,   1,-1, 0,
	0, 0,  -1, 1, 0,
	1, 0,   1, 1, 0,
    };

    glInterleavedArrays(GL_T2F_V3F, 0, vertexArray);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for (i = 0; i < frontend.frame_hz_divisor; i++)
	SDL_GL_SwapBuffers();
}

static void frontend_resize_window(void)
{
    unsigned width = LCD_WIDTH;
    unsigned height = LCD_HEIGHT;

    if (opt_visual_profiler)
	height += PROFILER_LINES;

    width *= frontend.scale;
    height *= frontend.scale;
    
    frontend.surface = SDL_SetVideoMode(width, height, 16, SDL_OPENGL);
    if (frontend.surface == NULL) {
	printf("Error creating SDL surface!\n");
	exit(1);
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

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    frontend_update_texture();
    frontend_draw_frame();
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
    SDL_WM_SetCaption("Thundercracker", NULL);
}

void frontend_exit(void)
{
    SDL_Quit();
}

static frontend_keydown(SDL_KeyboardEvent *evt)
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
	opt_visual_profiler = !opt_visual_profiler;
	frontend_resize_window();
	break;

    }
}

int frontend_loop(void)
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
	    
	    }
	}

	if (frontend.running) {
	    frontend_update_texture();
	    lcd_te_pulse();
	    frontend_draw_frame();
	}
    }
}

void frontend_async_quit(void)
{
    frontend.running = 0;
}
