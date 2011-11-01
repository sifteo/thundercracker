/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*- */

/* 8051 emulator 
 *
 * Copyright 2006 Jari Komppa
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 *
 * License for this file only:
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

#include <curses.h>
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

// Highest SP we've seen
uint8_t stackMax;

// currently active view
int view = NO_VIEW;

int breakpoint = -1;

// Cube that the debugger is currently attached to
Cube::Hardware *cube;

bool stopOnException = false;


void setSpeed(int speed, int runmode)
{  
    switch (speed)
    {
    case 7:
        slk_set(5, "+/-|1Hz", 0);
        cube->time->setTargetRate(1);
        break;
    case 6:
        slk_set(5, "+/-|5Hz", 0);
        cube->time->setTargetRate(5);
        break;
    case 5:
        slk_set(5, "+/-|30Hz", 0);
        cube->time->setTargetRate(30);
        break;
    case 4:
        slk_set(5, "+/-|1M", 0);
        cube->time->setTargetRate(1000000);
        break;
    case 3:
        slk_set(5, "+/-|2M", 0);
        cube->time->setTargetRate(2000000);
        break;
    case 2:
        slk_set(5, "+/-|5M", 0);
        cube->time->setTargetRate(5000000);
        break;
    case 1:
        slk_set(5, "+/-|f+", 0);
        cube->time->run();
        break;
    case 0:
        slk_set(5, "+/-|f*", 0);
        cube->time->run();
        break;
    }

    if (runmode == 0)
    {
        cube->time->stop();
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

static void change_view(CPU::em8051 *aCPU, int changeto)
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
        build_memeditor_view(cube);
        break;
    }
}

void refreshView()
{
    change_view(&cube->cpu, view);
}

void recordHistory()
{
    if (speed > 0) {
        CPU::em8051 *aCPU = &cube->cpu;

        icount++;

        historyline = (historyline + 1) % HISTORY_LINES;

        memcpy(history + (historyline * (128 + 64 + sizeof(int))), aCPU->mSFR, 128);
        memcpy(history + (historyline * (128 + 64 + sizeof(int))) + 128, aCPU->mData, 64);
        memcpy(history + (historyline * (128 + 64 + sizeof(int))) + 128 + 64, &aCPU->mPreviousPC, sizeof(int));
        
        uint8_t sp = aCPU->mSFR[REG_SP];
        if (sp > stackMax)
            stackMax = sp;
    }
}

void init()
{
    slk_init(1);
    if ( (initscr()) == NULL ) {
        fprintf(stderr, "Error initialising ncurses.\n");
        return;
    }

#ifdef PDCURSES
    resize_term(41, 80);
#endif

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
}

void attach(Cube::Hardware *_cube)
{
    cube = _cube;
    stackMax = 0;
    setSpeed(speed, runmode);
}

bool updateUI()
{
    CPU::em8051 *aCPU = &cube->cpu;
    bool step = false;

    while (!step) {
        int ch = getch();

        if (LINES != oldrows || COLS != oldcols)
            refreshView();

        if (runmode != 0)
            step = true;

        switch (ch) {
            
        case 'q':
        case 'Q':
            // Request quit
            return true;

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
            if (aCPU->mBreakpoint) {
                aCPU->mBreakpoint = 0;
                emu_popup(aCPU, "Breakpoint", "Breakpoint cleared.");
            } else {
                aCPU->mBreakpoint = emu_readvalue(aCPU, "Set Breakpoint", aCPU->mPC, 4);
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
            step = true;
            break;
        case 'r':
            if (runmode) {
                runmode = 0;
                setSpeed(speed, runmode);
            } else {
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
            emu_reset(aCPU);
            break;

        default:
            // by default, send keys to the current view
            switch (view) {
            case MAIN_VIEW:
                mainview_editor_keys(aCPU, ch);
                break;
            case MEMEDITOR_VIEW:
                memeditor_editor_keys(aCPU, ch);
                break;
            default:
                break;
            }
            break;
        }
    
        switch (view) {
        case MAIN_VIEW:
            mainview_update(cube);
            break;
        case MEMEDITOR_VIEW:
            memeditor_update(aCPU);
            break;
        default:
            break;
        }
    }

    return false;
}

void exit()
{
    endwin();
}

void writeProfile(CPU::em8051 *aCPU, const char *filename)
{
    CPU::profile_data *pd = aCPU->mProfileData;
    if (!pd)
        return;

    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Error opening profiler output file");
        return;
    }

    fprintf(f, "total_cycles  %%_cycles  fl_idle  loop_len  loop_count    addr   disassembly\n");

    for (int addr = 0; addr < CODE_SIZE; addr++, pd++) {
        if (pd->total_cycles) {
            char assembly[128];

            CPU::em8051_decode(aCPU, addr, assembly);
            fprintf(f, "%12d %8.4f%% %8d [%8d x %9d ]  %04x:  %s\n",
                    (int)pd->total_cycles,
                    (pd->total_cycles * 100) / (float)aCPU->vtime->clocks,
                    (int)pd->flash_idle,
                    (int)(pd->loop_hits ? pd->loop_cycles / pd->loop_hits : 0),
                    (int)pd->loop_hits,
                    addr, assembly);
        }
    }    

    fclose(f);
    fprintf(stderr, "Profiler output written to '%s'\n", filename);
}


};  // namespace Debug
};  // namespace Cube
