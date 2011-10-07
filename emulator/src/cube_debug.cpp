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

#include "cube_debug.h"

namespace Cube {
namespace Debug {

unsigned char history[HISTORY_SIZE];

// current line in the history cyclic buffers
int historyline = 0;
// last known columns and rows; for screen resize detection
int oldcols, oldrows;

int runmode = 1;
int speed = 1;

// instruction count; needed to replay history correctly
unsigned int icount = 0;

// current clock count
uint64_t clocks = 0;

enum viewMode {
    NO_VIEW = -1,
    MAIN_VIEW = 0,
    MEMEDITOR_VIEW = 1,
    NUM_VIEWS = 2,
};

// currently active view
int view = NO_VIEW;

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
    }
    view = changeto;

    if (COLS < 80 || LINES < 24) {
        werase(stdscr);
        waddstr(stdscr, "Screen is too small for the debugger!\n"
                "Please resize your terminal window.");
        view = NO_VIEW;
    } else if (view == NO_VIEW)
        view = MAIN_VIEW;

    switch (view)
    {
    case MAIN_VIEW:
        build_main_view(aCPU);
        break;
    case MEMEDITOR_VIEW:
        build_memeditor_view(aCPU);
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

int debug_main(void *arg)
{
    struct em8051 *aCPU = arg;
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

    do {
        if (LINES != oldrows || COLS != oldcols)
            refreshview(aCPU);

        switch (ch) {

        case KEY_F(1):
            change_view(aCPU, 0);
            break;
        case KEY_F(2):
            change_view(aCPU, 1);
            break;
        case 'v':
            change_view(aCPU, (view + 1) % NUM_VIEWS);
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
        }
    }
    while ( (ch = getch()) != 'Q' && emu_thread_running );

    endwin();
    frontend_async_quit();
    return 0;
}

int nodebug_main(void *arg)
{
    struct em8051 *aCPU = arg;

    runmode = 1;
    speed = 0;

    while (emu_thread_running)
        run_cycle_batch(aCPU);

    frontend_async_quit();
    return 0;
}

void profiler_write_disassembly(struct em8051 *aCPU, const char *filename)
{
    int addr;
    struct profile_data *pd = aCPU->mProfilerMem;
    FILE *f = fopen(filename, "w");

    if (!f) {
        perror("Error opening profiler output file");
        return;
    }

    fprintf(f, "total_cycles  %%_cycles  fl_idle  loop_len  loop_count    addr   disassembly\n");

    for (addr = 0; addr < CODE_SIZE; addr++, pd++) {
        if (pd->total_cycles) {
            char assembly[128];

            decode(aCPU, addr, assembly);
            fprintf(f, "%12lld %8.4f%% %8lld [%8lld x %9lld ]  %04x:  %s\n",
                    (long long int)pd->total_cycles,
                    (pd->total_cycles * 100) / (float)aCPU->profilerTotal,
                    (long long int)pd->flash_idle,
                    (long long int)(pd->loop_hits ? pd->loop_cycles / pd->loop_hits : 0),
                    (long long int)pd->loop_hits,
                    addr, assembly);
        }
    }    

    fclose(f);
    fprintf(stderr, "Profiler output written to '%s'\n", filename);
}

int main(int argc, char ** argv) 
{
                    runmode = 1;
                    speed = 1;
                    strcpy(filename, argv[i]);
                }
            }
        }
    }
   
    if (opt_trace_filename) {
        emu.traceFile = fopen(opt_trace_filename, "w");
        if (!emu.traceFile) {
            perror("Error opening trace file");
            return 1;
        }
    }

    hardware_init(&emu);
    frontend_init(&emu);

    // Emulation/debug main loop on another thread
    emu_thread_running = 1;
    emu_thread = SDL_CreateThread(opt_debug ? debug_main : nodebug_main, &emu);

    // GUI main loop on the main thread (SDL kind of requires this)
    frontend_loop();

    emu_thread_running = 0;
    SDL_WaitThread(emu_thread, NULL);

    frontend_exit();
    hardware_exit();

    if (opt_profile_filename)
        profiler_write_disassembly(&emu, opt_profile_filename);

    if (emu.traceFile)
        fclose(emu.traceFile);

    return 0;
}


};  // namespace Debug
};  // namespace Cube
