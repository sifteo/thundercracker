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
 * options.c
 * Options view
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curses.h"
#include "emu8051.h"
#include "emulator.h"

static int activerow = 0;

int opt_exception_iret_sp = 1;
int opt_exception_iret_acc = 1;
int opt_exception_iret_psw = 1;
int opt_exception_acc_to_a = 1;
int opt_exception_stack = 1;
int opt_exception_invalid = 1;
int opt_input_outputlow = 1;
int opt_clock_select = 6;
int opt_clock_hz = 16*1000*1000;
int opt_step_instruction = 1;

const char *opt_flash_filename = "flash.bin";
const char *opt_net_host = "127.0.0.1";
const char *opt_net_port = "2405";

int clockspeeds[] = { 
  1000*1000*1000,
   100*1000*1000, 
    72*1000*1000, 
    64*1000*1000, 
    32*1000*1000, 
    20*1000*1000,
    16*1000*1000,
     8*1000*1000,
     4*1000*1000,
     2*1000*1000,
       1000*1000,
        100*1000
};

void wipe_options_view()
{
}

void build_options_view(struct em8051 *aCPU)
{
    erase();
}

void options_editor_keys(struct em8051 *aCPU, int ch)
{
    switch (ch)
    {
    case KEY_UP:
        activerow--;        
        if (activerow < 0) 
            activerow = 0;
        /*
        if (activerow == 2)
            activerow = 1;
        */
        break;
    case KEY_DOWN:
        activerow++;
        if (activerow > 9) 
            activerow = 9;
        /*
        if (activerow == 2)
            activerow = 3;
            */
        break;
    case KEY_LEFT:
        switch (activerow)
        {
        case 1:
            opt_clock_select--;
            if (opt_clock_select < 0)
                opt_clock_select = 0;
            opt_clock_hz = clockspeeds[opt_clock_select];
            break;
        case 2:
            opt_input_outputlow--;
            if (opt_input_outputlow < 0)
                opt_input_outputlow = 0;
            break;
        case 3:
            opt_step_instruction = !opt_step_instruction;
            break;
        case 4:
            opt_exception_iret_sp = !opt_exception_iret_sp;
            break;
        case 5:
            opt_exception_iret_acc = !opt_exception_iret_acc;
            break;
        case 6:
            opt_exception_iret_psw = !opt_exception_iret_psw;
            break;
        case 7:
            opt_exception_acc_to_a = !opt_exception_acc_to_a;
            break;
        case 8:
            opt_exception_stack = !opt_exception_stack;
            break;
        case 9:
            opt_exception_invalid = !opt_exception_invalid;
            break;
        }
        break;
    case KEY_RIGHT:
        switch (activerow)
        {
        case 1:
            opt_clock_select++;
            if (opt_clock_select > 12)
                opt_clock_select = 12;
            if (opt_clock_select == 12)
                opt_clock_hz = emu_readhz(aCPU, "Enter custom clock speed", opt_clock_hz);
            else
                opt_clock_hz = clockspeeds[opt_clock_select];
            if (opt_clock_hz == 0) 
                opt_clock_hz = 1;
            break;
        case 2:
            opt_input_outputlow++;
            if (opt_input_outputlow > 2)
                opt_input_outputlow = 2;
            break;
        case 3:
            opt_step_instruction = !opt_step_instruction;
            break;
        case 4:
            opt_exception_iret_sp = !opt_exception_iret_sp;
            break;
        case 5:
            opt_exception_iret_acc = !opt_exception_iret_acc;
            break;
        case 6:
            opt_exception_iret_psw = !opt_exception_iret_psw;
            break;
        case 7:
            opt_exception_acc_to_a = !opt_exception_acc_to_a;
            break;
        case 8:
            opt_exception_stack = !opt_exception_stack;
            break;
        case 9:
            opt_exception_invalid = !opt_exception_invalid;
            break;
        }
        break;
    }
}

void options_update(struct em8051 *aCPU)
{
    int i;
    mvprintw(1, 1, "Options");
    for (i = 0; i < 10; i++)
        mvprintw(i * 2 + 3, 2, "  ");
    attron(A_REVERSE);
    mvprintw(3, 4, "< Hardware: nRF24LE1 >");
    mvprintw(5, 4, "< Clock at % 8.3f MHz >", opt_clock_hz / (1000*1000.0f));
    mvprintw(9, 4, "< Step steps %s >", opt_step_instruction ? "single instruction" : "single cpu cycle  ");
    /*
    mvprintw(7, 4, "< option >");
    */
    mvprintw(7, 4, "< High inputs from ports with low out level should be %c >", "01?"[opt_input_outputlow]);
    mvprintw(11, 4, "< Interrupt handler sp watch exception %s >", opt_exception_iret_sp?"enabled ":"disabled");
    mvprintw(13, 4, "< Interrupt handler acc watch exception %s >", opt_exception_iret_acc?"enabled ":"disabled");
    mvprintw(15, 4, "< Interrupt handler psw watch exception %s >", opt_exception_iret_psw?"enabled ":"disabled");
    mvprintw(17, 4, "< Acc-to-a opcode exception %s >", opt_exception_acc_to_a?"enabled ":"disabled");
    mvprintw(19, 4, "< Stack exception %s >", opt_exception_stack?"enabled ":"disabled");
    mvprintw(21, 4, "< Illegal opcode exception %s >", opt_exception_invalid?"enabled ":"disabled"); 
    attroff(A_REVERSE);
    mvprintw(activerow * 2 + 3, 2, "->");
}

