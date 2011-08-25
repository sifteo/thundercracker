/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <SDL.h>
#include "emu8051.h"
#include "emulator.h"
#include "frontend.h"
#include "lcd.h"

#define MARGIN           2
#define PROFILER_BYTES   16384
#define PROFILER_LINES   (2 * MARGIN + PROFILER_BYTES / LCD_WIDTH)
#define RGB565(r,g,b)    ((((r) >> 3)<<11) | (((g) >> 2)<<5) | ((b) >> 3))

static struct {
    SDL_Surface *surface;
    struct em8051 *cpu;
    int running;
    int scale;
} frontend;


static uint8_t clamp64(uint64_t val, uint64_t min, uint64_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

static void frontend_repaint(void)
{
    int x, y, i, j;
    uint16_t *src = lcd_framebuffer();
    uint16_t *dest = (uint16_t *) frontend.surface->pixels;
    uint16_t *s_line;

    SDL_LockSurface(frontend.surface);

    /*
     * Scaled LCD view
     */
    for (y = LCD_HEIGHT; y--;) {
	for (i = frontend.scale; i--;) {
	    s_line = src;
	    for (x = LCD_WIDTH; x--;) {
		for (j = frontend.scale; j--;)
		    *(dest++) = *s_line;
		s_line++;
	    }
	}
	src += LCD_WIDTH;
    }

    /*
     * Visual profiler
     */
    if (opt_visual_profiler) {
	static uint64_t shadow[PROFILER_BYTES];
	uint64_t *profiler = frontend.cpu->mProfilerMem;
	uint64_t *shadow_p = shadow;

	dest += LCD_WIDTH * frontend.scale * MARGIN;

	for (shadow_p = shadow; shadow_p < shadow + PROFILER_BYTES; shadow_p++) {
	    uint64_t count = *profiler - *shadow_p;
	    uint64_t heat_r = count ? clamp64(count / 1, 64, 255) : 0;
	    uint64_t heat_gb = count ? clamp64(count / 10, 64, 255) : 0;
	    uint16_t pixel = RGB565(heat_r, heat_gb, heat_gb);

	    *shadow_p = *profiler; 
	    profiler++;
	    for (j = frontend.scale; j--;)
		*(dest++) = pixel;		    
	}
    }

    SDL_UnlockSurface(frontend.surface);
    SDL_Flip(frontend.surface);
}

void frontend_resize_window(void)
{
    unsigned width = LCD_WIDTH * frontend.scale;
    unsigned height = LCD_HEIGHT * frontend.scale;

    if (opt_visual_profiler)
	height += PROFILER_LINES;
    
    frontend.surface = SDL_SetVideoMode(width, height, 16, 0);
    if (frontend.surface == NULL) {
	printf("Error creating SDL surface!\n");
	exit(1);
    }

    frontend_repaint();
}

void frontend_init(struct em8051 *cpu)
{
    SDL_Init(SDL_INIT_VIDEO);
    frontend.running = 1;
    frontend.scale = 2;
    frontend.cpu = cpu;

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
	    // Nap if we're idle, otherwise update the screen
	    if (lcd_check_for_repaint())
		frontend_repaint();
	    else
		SDL_Delay(10);
	}
    }
}

void frontend_async_quit(void)
{
    frontend.running = 0;
}
