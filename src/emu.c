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
// are we in single-step or run mode
int runmode = 0;
// current run speed, lower is faster
int speed = 1;

// instruction count; needed to replay history correctly
unsigned int icount = 0;

// current clock count
unsigned int clocks = 0;

// currently active view
int view = MAIN_VIEW;

// old port out values
int p0out = 0;
int p1out = 0;
int p2out = 0;
int p3out = 0;

int breakpoint = -1;

// returns time in 1ms units
int getTick()
{
#ifdef _MSC_VER
    return GetTickCount();
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
#endif
}

void emu_sleep(int value)
{
#ifdef _MSC_VER
    Sleep(value);
#else
    usleep(value * 1000);
#endif
}

void setSpeed(int speed, int runmode)
{   
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
    }

    if (speed < 4)
    {
        nocbreak();
        cbreak();
        nodelay(stdscr, TRUE);
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
    case LOGICBOARD_VIEW:
        wipe_logicboard_view();
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
    case LOGICBOARD_VIEW:
        build_logicboard_view(aCPU);
        break;
    case MEMEDITOR_VIEW:
        build_memeditor_view(aCPU);
        break;
    case OPTIONS_VIEW:
        build_options_view(aCPU);
        break;
    }
}

int main(int argc, char ** argv) 
{
    int ch = 0;
    struct em8051 emu;
    int i;
    int ticked = 1;

    memset(&emu, 0, sizeof(emu));
    emu.mCodeMem     = malloc(65536);
    emu.mCodeMemSize = 65536;
    emu.mExtData     = malloc(65536);
    emu.mExtDataSize = 65536;
    emu.mLowerData   = malloc(128);
    emu.mUpperData   = malloc(128);
    emu.mSFR         = malloc(128);
    emu.except       = &emu_exception;
    emu.sfrwrite     = &hardware_sfrwrite;
    emu.xread = NULL;
    emu.xwrite = NULL;
    reset(&emu, 1);    

    if (argc > 1)
    {
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (strcmp("step_instruction",argv[i]+1) == 0)
                {
                    opt_step_instruction = 1;
                }
                else
                if (strcmp("si",argv[i]+1) == 0)
                {
                    opt_step_instruction = 1;
                }
                else
                if (strcmp("noexc_iret_sp",argv[i]+1) == 0)
                {
                    opt_exception_iret_sp = 0;
                }
                else
                if (strcmp("nosp",argv[i]+1) == 0)
                {
                    opt_exception_iret_sp = 0;
                }
                else
                if (strcmp("noexc_iret_acc",argv[i]+1) == 0)
                {
                    opt_exception_iret_acc = 0;
                }
                else
                if (strcmp("noacc",argv[i]+1) == 0)
                {
                    opt_exception_iret_acc = 0;
                }
                else
                if (strcmp("noexc_iret_psw",argv[i]+1) == 0)
                {
                    opt_exception_iret_psw = 0;
                }
                else
                if (strcmp("nopsw",argv[i]+1) == 0)
                {
                    opt_exception_iret_psw = 0;
                }
                else
                if (strcmp("noexc_acc_to_a",argv[i]+1) == 0)
                {
                    opt_exception_acc_to_a = 0;
                }
                else
                if (strcmp("noaa",argv[i]+1) == 0)
                {
                    opt_exception_acc_to_a = 0;
                }
                else
                if (strcmp("noexc_stack",argv[i]+1) == 0)
                {
                    opt_exception_stack = 0;
                }
                else
                if (strcmp("nostk",argv[i]+1) == 0)
                {
                    opt_exception_stack = 0;
                }
                else
                if (strcmp("noexc_invalid_op",argv[i]+1) == 0)
                {
                    opt_exception_invalid = 0;
                }
                else
                if (strcmp("noiop",argv[i]+1) == 0)
                {
                    opt_exception_invalid = 0;
                }
                else
                if (strncmp("clock=",argv[i]+1,6) == 0)
                {
                    opt_clock_select = 12;
                    opt_clock_hz = atoi(argv[i]+7);
                    if (opt_clock_hz <= 0)
                        opt_clock_hz = 1;
                }
                if (strncmp("flash=",argv[i]+1,6) == 0)
                {
		    opt_flash_filename = argv[i]+7;
                }
                else
                {
                    printf("Help:\n\n"
			   "%s [options] [firmware.ihx]\n\n"
			   "Both the filename and options are optional. Available options:\n\n"
			   "Option            Alternate   description\n"
			   "-step_instruction -si         Step one instruction at a time\n"
			   "-noexc_iret_sp    -nosp       Disable sp iret exception\n"
			   "-noexc_iret_acc   -noacc      Disable acc iret exception\n"
			   "-noexc_iret_psw   -nopsw      Disable pdw iret exception\n"
			   "-noexc_acc_to_a   -noaa       Disable acc-to-a invalid instruction exception\n"
			   "-noexc_stack      -nostk      Disable stack abnormal behaviour exception\n"
			   "-noexc_invalid_op -noiop      Disable invalid opcode exception\n"
			   "-iolowlow         If out pin is low, hi input from same pin is low\n"
			   "-iolowrand        If out pin is low, hi input from same pin is random\n"
			   "-clock=value      Set clock speed, in Hz\n"
			   "-flash=file.bin   Set Flash memory filename\n",
			   argv[0]);
                    return -1;
                }
            }
            else
            {
                if (load_obj(&emu, argv[i]) != 0)
                {
                    printf("File '%s' load failure\n\n",argv[i]);
                    return -1;
                }
                else
                {
                    strcpy(filename, argv[i]);
                }
            }
        }
    }

    //  Initialize ncurses

    slk_init(1);
    if ( (initscr()) == NULL ) {
	    fprintf(stderr, "Error initialising ncurses.\n");
	    exit(EXIT_FAILURE);
    }
 
    slk_set(1, "h)elp", 0);
    slk_set(2, "l)oad", 0);
    slk_set(3, "spc=step", 0);
    slk_set(4, "r)un", 0);
    slk_set(6, "v)iew", 0);
    slk_set(7, "home=rst", 0);
    slk_set(8, "s-Q)quit", 0);
    setSpeed(speed, runmode);
   

    //  Switch of echoing and enable keypad (for arrow keys etc)

    cbreak(); // no buffering
    noecho(); // no echoing
    keypad(stdscr, TRUE); // cursors entered as single characters

    build_main_view(&emu);
    hardware_init(&emu);

    // Loop until user hits 'shift-Q'

    do
    {
        if (LINES != oldrows ||
            COLS != oldcols)
        {
            refreshview(&emu);
        }
        switch (ch)
        {
        case KEY_F(1):
            change_view(&emu, 0);
            break;
        case KEY_F(2):
            change_view(&emu, 1);
            break;
        case KEY_F(3):
            change_view(&emu, 2);
            break;
        case KEY_F(4):
            change_view(&emu, 3);
            break;
        case 'v':
            change_view(&emu, (view + 1) % 4);
            break;
        case 'k':
            if (breakpoint != -1)
            {
                breakpoint = -1;
                emu_popup(&emu, "Breakpoint", "Breakpoint cleared.");
            }
            else
            {
                breakpoint = emu_readvalue(&emu, "Set Breakpoint", emu.mPC, 4);
            }
            break;
        case 'g':
            emu.mPC = emu_readvalue(&emu, "Set Program Counter", emu.mPC, 4);
            break;
        case 'h':
            emu_help(&emu);
            break;
        case 'l':
            emu_load(&emu);
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
            if (emu_reset(&emu))
            {
                clocks = 0;
                ticked = 1;
            }
            break;
        case KEY_END:
            clocks = 0;
            ticked = 1;
            break;
        default:
            // by default, send keys to the current view
            switch (view)
            {
            case MAIN_VIEW:
                mainview_editor_keys(&emu, ch);
                break;
            case LOGICBOARD_VIEW:
                logicboard_editor_keys(&emu, ch);
                break;
            case MEMEDITOR_VIEW:
                memeditor_editor_keys(&emu, ch);
                break;
            case OPTIONS_VIEW:
                options_editor_keys(&emu, ch);
                break;
            }
            break;
        }

        if (ch == 32 || runmode)
        {
            int targettime;
            unsigned int targetclocks;
            targetclocks = 1;
            targettime = getTick();

            if (speed == 2 && runmode)
            {
                targettime += 1;
                targetclocks += (opt_clock_hz / 16000) - 1;
            }
            if (speed < 2 && runmode)
            {
		targettime += 30;  // Run for 30ms between display refreshes
                targetclocks += (opt_clock_hz / 10) - 1;
            }

            do
            {
                int old_pc;
                old_pc = emu.mPC;
                if (opt_step_instruction)
                {
                    ticked = 0;
                    while (!ticked)
                    {
                        targetclocks--;
                        clocks++;
                        ticked = tick(&emu);
                        logicboard_tick(&emu);
                    }
                }
                else
                {
                    targetclocks--;
                    clocks++;
		    ticked = tick(&emu);
                    logicboard_tick(&emu);
                }

                if (emu.mPC == breakpoint)
                    emu_exception(&emu, -1);

                if (ticked)
                {
                    icount++;

                    historyline = (historyline + 1) % HISTORY_LINES;

                    memcpy(history + (historyline * (128 + 64 + sizeof(int))), emu.mSFR, 128);
                    memcpy(history + (historyline * (128 + 64 + sizeof(int))) + 128, emu.mLowerData, 64);
                    memcpy(history + (historyline * (128 + 64 + sizeof(int))) + 128 + 64, &old_pc, sizeof(int));
                }
            }
            while (targettime > getTick() && targetclocks > 0);

            while (targettime > getTick())
            {
                emu_sleep(1);
            }
        }

        switch (view)
        {
        case MAIN_VIEW:
            mainview_update(&emu);
            break;
        case LOGICBOARD_VIEW:
            logicboard_update(&emu);
            break;
        case MEMEDITOR_VIEW:
            memeditor_update(&emu);
            break;
        case OPTIONS_VIEW:
            options_update(&emu);
            break;
        }
    }
    while ( (ch = getch()) != 'Q' );

    endwin();
    hardware_exit();

    return EXIT_SUCCESS;
}
