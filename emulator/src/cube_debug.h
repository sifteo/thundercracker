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

#ifndef _CUBE_DEBUG_H
#define _CUBE_DEBUG_H

#include <stdint.h>

#include "vtime.h"
#include "cube_cpu.h"
#include "cube_hardware.h"

namespace Cube {
namespace Debug {

#define HISTORY_LINES  20
#define HISTORY_SIZE   (HISTORY_LINES * (128 + 64 + sizeof(int)))
#define FILENAME_SIZE  256

enum EMU_VIEWS
{
    NO_VIEW = -1,
    MAIN_VIEW = 0,
    MEMEDITOR_VIEW,
    NUM_VIEWS,
};

// binary history buffer
extern unsigned char history[HISTORY_SIZE];

// last used filename
extern char filename[FILENAME_SIZE];

// instruction count; needed to replay history correctly
extern unsigned int icount;

// current line in the history cyclic buffers
extern int historyline;

// last known columns and rows; for screen resize detection
extern int oldcols, oldrows;

// are we in single-step or run mode
extern int runmode;

// current run speed, lower is faster
extern int speed;

// currently active view
extern int view;

// main
void init(Cube::Hardware *cube);
void exit();
void writeProfile(CPU::em8051 *aCPU, const char *filename);
void updateUI();
void recordHistory();
void refreshView();
void setSpeed(int speed, int runmode);

// popups
void emu_help(CPU::em8051 *aCPU);
int emu_reset(CPU::em8051 *aCPU);
int emu_readvalue(CPU::em8051 *aCPU, const char *aPrompt, int aOldvalue, int aValueSize);
int emu_readhz(CPU::em8051 *aCPU, const char *aPrompt, int aOldvalue);
void emu_load(CPU::em8051 *aCPU);
void emu_exception(CPU::em8051 *aCPU, int aCode);
void emu_popup(CPU::em8051 *aCPU, const char *aTitle, const char *aMessage);

// mainview
void mainview_editor_keys(CPU::em8051 *aCPU, int ch);
void build_main_view(CPU::em8051 *aCPU);
void wipe_main_view();
void mainview_update(Cube::Hardware *cube);

// memeditor.c
void wipe_memeditor_view();
void build_memeditor_view(Cube::Hardware *cube);
void memeditor_editor_keys(CPU::em8051 *aCPU, int ch);
void memeditor_update(CPU::em8051 *aCPU);


};  // namespace Debug
};  // namespace Cube

#endif
