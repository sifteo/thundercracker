/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*- */

/* 8051 emulator 
 * Copyright 2006 Jari Komppa
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject 
 * to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE. 
 *
 * (i.e. the MIT License)
 *
 * emu.c
 * Curses-based emulator front-end
 */

#include <SDL.h>   // For SDL's main() wrapper

#ifdef _MSC_VER
#include <windows.h>
#undef MOUSE_MOVED
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curses.h"
#include "emu8051.h"
#include "emulator.h"
#include "hardware.h"
#include "lcd.h"

unsigned char history[HISTORY_LINES * (128 + 64 + sizeof(int))];


// current line in the history cyclic buffers
int historyline = 0;
// last known columns and rows; for screen resize detection
int oldcols, oldrows;

/*
 * XXX: UI defaults to running at 10Hz when no firmware is loaded,
 *      not because this is really useful, but because the SDL
 *      event loop is wedged any time we're actually single-stepping.
 *      This is especially annoying on Windows, when we are greeted
 *      with an unresponsive and immovable window.
 *
 *      These defaults are overridden when a firmware file is loaded
 *      on the command line, so that we then run at maximum speed.
 */
int runmode = 1;
int speed = 4;

// instruction count; needed to replay history correctly
unsigned int icount = 0;

// current clock count
uint64_t clocks = 0;

// currently active view
int view = MAIN_VIEW;

// old port out values
int p0out = 0;
int p1out = 0;
int p2out = 0;
int p3out = 0;

int breakpoint = -1;

uint64_t target_clocks;
uint32_t target_time;

void setSpeed(int speed, int runmode)
{   
    // Reset timebase
    target_clocks = clocks;
    target_time = SDL_GetTicks();

    switch (speed)
    {
    case 7:
        slk_set(5, "+/-|.5Hz", 0);
        break;
    case 6:
        slk_set(5, "+/-|1Hz", 0);
        break;
    case 5:
        slk_set(5, "+/-|2Hz", 0);
        break;
    case 4:
        slk_set(5, "+/-|10Hz", 0);
        break;
    case 3:
        slk_set(5, "+/-|fast", 0);
        break;
    case 2:
        slk_set(5, "+/-|f+", 0);
        break;
    case 1:
        slk_set(5, "+/-|f++", 0);
        break;
    case 0:
        slk_set(5, "+/-|f*", 0);
        break;
    }

    if (runmode == 0)
    {
        slk_set(4, "r)un", 0);
        slk_refresh();        
        nocbreak();
        cbreak();

	/*
	 * GRR. This will block the whole main loop, which blocks the
	 * SDL event loop on Mac/Win hosts.  But this blocking
	 * behaviour is expected by all the popup code.
	 *
	 * I should really try and get the ncurses UI separated out
	 * onto a separate thread...  --beth
	 */
	nodelay(stdscr, FALSE); 
        return;
    }
    else
    {
        slk_set(4, "r)unning", 0);
        slk_refresh();        
	nodelay(stdscr, TRUE);
    }

    if (speed < 4)
    {
        nocbreak();
        cbreak();
    }
    else
    {
        switch(speed)
        {
	case 7:
            halfdelay(20);
            break;
        case 6:
            halfdelay(10);
            break;
        case 5:
            halfdelay(5);
            break;
        case 4:
            halfdelay(1);
            break;
        }
    }
}

void refreshview(struct em8051 *aCPU)
{
    change_view(aCPU, view);
}

void change_view(struct em8051 *aCPU, int changeto)
{
    switch (view)
    {
    case MAIN_VIEW:
        wipe_main_view();
        break;
    case MEMEDITOR_VIEW:
        wipe_memeditor_view();
        break;
    case OPTIONS_VIEW:
        wipe_options_view();
        break;
    }
    view = changeto;
    switch (view)
    {
    case MAIN_VIEW:
        build_main_view(aCPU);
        break;
    case MEMEDITOR_VIEW:
        build_memeditor_view(aCPU);
        break;
    case OPTIONS_VIEW:
        build_options_view(aCPU);
        break;
    }
}

void run_cycle_batch(struct em8051 *aCPU)
{
    /*
     * Put more time on the clock. This accumulates at every
     * iteration, so we'll compensate for any momentary errors
     * and settle on the correct average speed.
     */
    
    if (speed == 2 && runmode)
	{
	    target_time += 1;
	    target_clocks += opt_clock_hz / 16000;
	}

    if (speed < 2 && runmode)
	{
	    // Run for at least 30ms between display refreshes
	    const int quanta = 30;

	    target_time += quanta;
	    target_clocks += opt_clock_hz / 1000 * quanta;
	}

    do
	{
            int old_pc = aCPU->mPC;
	    int ticked;

	    if (speed == 0) {
		/*
		 * Fastest speed. Run ticks in large batches, so we don't spend
		 * all this CPU time in silly places like SDL_GetTicks() or ncurses.
		 */

		while (target_clocks > clocks) {
		    clocks++;
		    tick(aCPU);
		}
		ticked = 0;
	    }
	    else if (opt_step_instruction) {
		/* Keep running until we actually execute an instruction */

		ticked = 0;
		while (!ticked) {
		    clocks++;
		    ticked = tick(aCPU);
		}

		if (aCPU->mPC == breakpoint)
		    emu_exception(aCPU, -1);
	    }
	    else {
		/*
		 * Generic, run one clock cycle.
		 */

		clocks++;
		ticked = tick(aCPU);
	    }

	    // Update history in all speeds but the fastest, if we ran an actual instruction
	    if (ticked) {
		icount++;

		historyline = (historyline + 1) % HISTORY_LINES;

		memcpy(history + (historyline * (128 + 64 + sizeof(int))), aCPU->mSFR, 128);
		memcpy(history + (historyline * (128 + 64 + sizeof(int))) + 128, aCPU->mLowerData, 64);
		memcpy(history + (historyline * (128 + 64 + sizeof(int))) + 128 + 64, &old_pc, sizeof(int));
	    }
	}
    while ((int32_t)(target_time - SDL_GetTicks()) > 0
	   && target_clocks > clocks);
	    
    // Running too fast? Slow down a bit!

    while ((int32_t)(target_time - SDL_GetTicks()) > 0)
        SDL_Delay(1);
}

void debug_main(struct em8051 *aCPU)
{
    int ch = 0;
    int ticked = 1;

    slk_init(1);
    if ( (initscr()) == NULL ) {
	    fprintf(stderr, "Error initialising ncurses.\n");
	    exit(EXIT_FAILURE);
    }

    cbreak(); // no buffering
    noecho(); // no echoing
    keypad(stdscr, TRUE); // cursors entered as single characters

    slk_set(1, "h)elp", 0);
    slk_set(2, "l)oad", 0);
    slk_set(3, "spc=step", 0);
    slk_set(4, "r)un", 0);
    slk_set(6, "v)iew", 0);
    slk_set(7, "home=rst", 0);
    slk_set(8, "s-Q)quit", 0);
    setSpeed(speed, runmode);

    // Draw the first screen
    build_main_view(aCPU);

    do
    {
	// Need to handle SDL events on the main thread!
	if (lcd_eventloop())
	    break;

        if (LINES != oldrows ||
            COLS != oldcols)
        {
            refreshview(aCPU);
        }
        switch (ch)
        {
        case KEY_F(1):
            change_view(aCPU, 0);
            break;
        case KEY_F(2):
            change_view(aCPU, 1);
            break;
        case KEY_F(3):
            change_view(aCPU, 2);
            break;
        case 'v':
            change_view(aCPU, (view + 1) % 3);
            break;
        case 'k':
            if (breakpoint != -1)
            {
                breakpoint = -1;
                emu_popup(aCPU, "Breakpoint", "Breakpoint cleared.");
            }
            else
            {
                breakpoint = emu_readvalue(aCPU, "Set Breakpoint", aCPU->mPC, 4);
            }
            break;
        case 'g':
            aCPU->mPC = emu_readvalue(aCPU, "Set Program Counter", aCPU->mPC, 4);
            break;
        case 'h':
            emu_help(aCPU);
            break;
        case 'l':
            emu_load(aCPU);
            break;
        case ' ':
            runmode = 0;
            setSpeed(speed, runmode);
            break;
        case 'r':
            if (runmode)
            {
                runmode = 0;
                setSpeed(speed, runmode);
            }
            else
            {
                runmode = 1;
                setSpeed(speed, runmode);
            }
            break;
#ifdef __PDCURSES__
        case PADPLUS:
#endif
        case '+':
        case '=':   // + without shift :)
            speed--;
            if (speed < 0)
                speed = 0;
            setSpeed(speed, runmode);
            break;
#ifdef __PDCURSES__
        case PADMINUS:
#endif
        case '-':
            speed++;
            if (speed > 7)
                speed = 7;
            setSpeed(speed, runmode);
            break;
        case KEY_HOME:
            if (emu_reset(aCPU))
            {
                target_clocks = clocks = 0;
                ticked = 1;
            }
            break;
        case KEY_END:
            target_clocks = clocks = 0;
            ticked = 1;
            break;
        default:
            // by default, send keys to the current view
            switch (view)
            {
            case MAIN_VIEW:
                mainview_editor_keys(aCPU, ch);
                break;
            case MEMEDITOR_VIEW:
                memeditor_editor_keys(aCPU, ch);
                break;
            case OPTIONS_VIEW:
                options_editor_keys(aCPU, ch);
                break;
            }
            break;
        }

        if (ch == 32 || runmode)
	    run_cycle_batch(aCPU);

        switch (view)
        {
        case MAIN_VIEW:
            mainview_update(aCPU);
            break;
        case MEMEDITOR_VIEW:
            memeditor_update(aCPU);
            break;
        case OPTIONS_VIEW:
            options_update(aCPU);
            break;
        }
    }
    while ( (ch = getch()) != 'Q' );

    endwin();
}

void nodebug_main(struct em8051 *aCPU)
{
    runmode = 1;
    speed = 0;

    while (!lcd_eventloop())
	run_cycle_batch(aCPU);
}

void profiler_write_disassembly(struct em8051 *aCPU, const char *filename)
{
    int addr;
    FILE *f = fopen(filename, "w");

    if (!f) {
	perror("Error opening profiler output file");
	return;
    }

    for (addr = 0; addr < aCPU->mCodeMemSize; addr++) {
	uint64_t count = aCPU->mProfilerMem[addr];

	if (count) {
	    char assembly[128];

            decode(aCPU, addr, assembly);
	    fprintf(f, "%04X: % 12lld % 8.4f%%   %s\n",
		    addr, (long long int)count,
		    (count * 100) / (float)aCPU->profilerTotal,
		    assembly);
	}
    }    

    fclose(f);
    fprintf(stderr, "Profiler output written to '%s'\n", filename);
}

int main(int argc, char ** argv) 
{
    struct em8051 emu;
    int i;
 
    memset(&emu, 0, sizeof(emu));
    emu.mExtDataSize = 1024;
    emu.mCodeMemSize = 16*1024;
    emu.mCodeMem     = calloc(1, emu.mCodeMemSize);
    emu.mProfilerMem = calloc(sizeof emu.mProfilerMem[0], emu.mCodeMemSize);
    emu.mExtData     = calloc(1, emu.mExtDataSize);
    emu.mLowerData   = calloc(1, 128);
    emu.mUpperData   = calloc(1, 128);
    emu.mSFR         = calloc(1, 128);
    emu.except       = &emu_exception;
    emu.sfrwrite     = &hardware_sfrwrite;
    emu.sfrread      = &hardware_sfrread;
    emu.xread = NULL;
    emu.xwrite = NULL;
    reset(&emu, 1);    
 
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (strcmp("vp",argv[i]+1) == 0)
                    opt_visual_profiler = 1;
                else if (strcmp("d",argv[i]+1) == 0)
                    opt_debug = 1;
		else if (strncmp("clock=",argv[i]+1,6) == 0) {
                    opt_clock_select = 12;
                    opt_clock_hz = atoi(argv[i]+7);
                    if (opt_clock_hz <= 0)
                        opt_clock_hz = 1;
                } 
		else if (strncmp("flash=",argv[i]+1,6) == 0)
		    opt_flash_filename = argv[i]+7;
		else if (strncmp("profile=",argv[i]+1,8) == 0)
		    opt_profile_filename = argv[i]+9;
                else if (strncmp("host=",argv[i]+1,5) == 0)
		    opt_net_host = argv[i]+6;
                else if (strncmp("port=",argv[i]+1,5) == 0)
		    opt_net_port = argv[i]+6;
                else {
                    printf("Help:\n\n"
			   "%s [options] [firmware.ihx]\n\n"
			   "Both the filename and options are optional. Available options:\n"
			   "\n"
			   "-d          Enable ncurses debug UI\n"
			   "-vp         Visual profiler mode, shows realtime CPU cycle heat-map\n"
			   "\n"
			   "-profile=out.txt  Profile performance, and write annotated disassembly\n"
			   "-clock=value      Set clock speed, in Hz\n"
			   "-flash=file.bin   Set Flash memory filename\n",
			   "-host=hostname    Hostname for nethub connection\n",
			   "-port=port        Port number for nethub connection\n",
			   argv[0]);
                    return -1;
                }
            } else {
                if (load_obj(&emu, argv[i]) != 0) {
                    printf("File '%s' load failure\n\n",argv[i]);
                    return -1;
                } else {
		    /*
		     * Loaded firmware successfully. Remember the
		     * file, and start running it full-speed!
		     */
		    
		    runmode = 1;
		    speed = 1;
                    strcpy(filename, argv[i]);
                }
            }
        }
    }
   
    /*
     * Init hardware earlyish. Initializing the LCD controller will create an SDL window.
     *
     * On Mac OS, due to some ugly SDL threading quirks, we get some
     * warnings on the console during the first redraw. We would
     * prefer to get the unpleasantness over with early, then repaint
     * over the screen when we init ncurses.  On a less kludgey note,
     * this also allows us to bail out if there's an error without
     * having to worry about putting the console back into a usable
     * state.
     */
    hardware_init(&emu);

    if (opt_debug)
	debug_main(&emu);
    else
	nodebug_main(&emu);

    hardware_exit();

    if (opt_profile_filename)
	profiler_write_disassembly(&emu, opt_profile_filename);

    return 0;
}
