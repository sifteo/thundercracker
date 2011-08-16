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

// how many lines of history to remember
#define HISTORY_LINES 20

enum EMU_VIEWS
{
    MAIN_VIEW = 0,
    MEMEDITOR_VIEW = 1,
    LOGICBOARD_VIEW = 2,
    OPTIONS_VIEW = 3
};


// binary history buffer
extern unsigned char history[];

// last used filename
extern char filename[];

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

// old port out values
extern int p0out;
extern int p1out;
extern int p2out;
extern int p3out;

// current clock count
extern unsigned int clocks;

extern int opt_exception_iret_sp;
extern int opt_exception_iret_acc;
extern int opt_exception_iret_psw;
extern int opt_exception_acc_to_a;
extern int opt_exception_stack;
extern int opt_exception_invalid;
extern int opt_clock_select;
extern int opt_clock_hz;
extern int opt_step_instruction;
extern int opt_input_outputlow;

extern const char *opt_flash_filename;


// emu.c
extern int getTick();
extern void setSpeed(int speed, int runmode);
extern int emu_sfrread(struct em8051 *aCPU, int aRegister);
extern void refreshview(struct em8051 *aCPU);
extern void change_view(struct em8051 *aCPU, int changeto);

// popups.c
extern void emu_help(struct em8051 *aCPU);
extern int emu_reset(struct em8051 *aCPU);
extern int emu_readvalue(struct em8051 *aCPU, const char *aPrompt, int aOldvalue, int aValueSize);
extern int emu_readhz(struct em8051 *aCPU, const char *aPrompt, int aOldvalue);
extern void emu_load(struct em8051 *aCPU);
extern void emu_exception(struct em8051 *aCPU, int aCode);
extern void emu_popup(struct em8051 *aCPU, char *aTitle, char *aMessage);

// mainview.c
extern void mainview_editor_keys(struct em8051 *aCPU, int ch);
extern void build_main_view(struct em8051 *aCPU);
extern void wipe_main_view();
extern void mainview_update(struct em8051 *aCPU);

// logicboard.c
extern void wipe_logicboard_view();
extern void build_logicboard_view(struct em8051 *aCPU);
extern void logicboard_editor_keys(struct em8051 *aCPU, int ch);
extern void logicboard_update(struct em8051 *aCPU);
extern void logicboard_tick(struct em8051 *aCPU);

// memeditor.c
extern void wipe_memeditor_view();
extern void build_memeditor_view(struct em8051 *aCPU);
extern void memeditor_editor_keys(struct em8051 *aCPU, int ch);
extern void memeditor_update(struct em8051 *aCPU);

// options.c
extern void wipe_options_view();
extern void build_options_view(struct em8051 *aCPU);
extern void options_editor_keys(struct em8051 *aCPU, int ch);
extern void options_update(struct em8051 *aCPU);




