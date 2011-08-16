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
 * memeditor.c
 * Memory editor view
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curses.h"
#include "emu8051.h"
#include "emulator.h"

struct memeditor
{
    WINDOW *box, *view;
    unsigned char *memarea;
    int lines;
    int cursorpos;
    int memoffset;
    int maxmem;
    int memviewoffset;
};

static struct memeditor eds[5];
static int focus = 0;

void wipe_memeditor_view()
{
    int i;
    for (i = 0; i < 5; i++)
    {
        delwin(eds[i].view);
        delwin(eds[i].box);
    }
}

void build_memeditor_view(struct em8051 *aCPU)
{
    erase();
    
    eds[0].lines = (LINES / 3);
    eds[0].box = subwin(stdscr, eds[0].lines, 40, 0, 0);
    box(eds[0].box,ACS_VLINE,ACS_HLINE);
    mvwaddstr(eds[0].box, 0, 2, "Lower");
    eds[0].view = subwin(eds[0].box, eds[0].lines - 2, 38, 1, 1);
    eds[0].maxmem = 128;
    eds[0].memarea = aCPU->mLowerData;
    eds[0].memviewoffset = 0;

    eds[1].lines = (LINES / 3);
    eds[1].box = subwin(stdscr, eds[1].lines, 40, eds[0].lines, 0);
    box(eds[1].box,ACS_VLINE,ACS_HLINE);
    mvwaddstr(eds[1].box, 0, 2, "Upper");
    eds[1].view = subwin(eds[1].box, eds[1].lines - 2, 38, eds[0].lines + 1, 1);
    if (aCPU->mUpperData)
        eds[1].maxmem = 128;
    else
        eds[1].maxmem = 0;
    eds[1].memarea = aCPU->mUpperData;
    eds[1].memviewoffset = 128;

    eds[2].lines = LINES - (eds[0].lines + eds[1].lines);
    eds[2].box = subwin(stdscr, eds[2].lines, 40, eds[0].lines + eds[1].lines, 0);
    box(eds[2].box,ACS_VLINE,ACS_HLINE);
    mvwaddstr(eds[2].box, 0, 2, "SFR");
    eds[2].view = subwin(eds[2].box, eds[2].lines - 2, 38, eds[0].lines + eds[1].lines + 1, 1);
    eds[2].maxmem = 128;
    eds[2].memarea = aCPU->mSFR;
    eds[2].memviewoffset = 128;

    eds[3].lines = LINES / 2;
    eds[3].box = subwin(stdscr, eds[3].lines, 40, 0, 40);
    box(eds[3].box,ACS_VLINE,ACS_HLINE);
    mvwaddstr(eds[3].box, 0, 2, "External");
    eds[3].view = subwin(eds[3].box, eds[3].lines - 2, 38, 1, 41);    
    eds[3].maxmem = aCPU->mExtDataSize;
    eds[3].memarea = aCPU->mExtData;
    eds[3].memviewoffset = 0;

    eds[4].lines = LINES / 2;
    eds[4].box = subwin(stdscr, eds[3].lines, 40, eds[3].lines, 40);
    box(eds[4].box,ACS_VLINE,ACS_HLINE);
    mvwaddstr(eds[4].box, 0, 2, "ROM");
    eds[4].view = subwin(eds[4].box, eds[4].lines - 2, 38, eds[3].lines + 1, 41);
    eds[4].maxmem = aCPU->mCodeMemSize;
    eds[4].memarea = aCPU->mCodeMem;     
    eds[4].memviewoffset = 0;

    // TODO: make sure cursorpos / memoffset are within legal values,
    // but don't mess them up otherwise

    eds[0].cursorpos = 0;
    eds[1].cursorpos = 0;
    eds[2].cursorpos = 0;
    eds[3].cursorpos = 0;
    eds[4].cursorpos = 0;

    eds[0].memoffset = 0;
    eds[1].memoffset = 0;
    eds[2].memoffset = 0;
    eds[3].memoffset = 0;
    eds[4].memoffset = 0;

    refresh();
}

void memeditor_editor_keys(struct em8051 *aCPU, int ch)
{
    int insert_value = -1;
    switch(ch)
    {
    case KEY_NEXT:
    case '\t':
        focus++;
        if (focus == 1 && aCPU->mUpperData == NULL) 
            focus++;
        if (focus == 3 && aCPU->mExtDataSize == 0)
            focus++;
        if (focus == 5)
            focus = 0;
        break;
    case KEY_NPAGE:
        eds[focus].cursorpos += (eds[focus].lines - 2) * 16;
        break;
    case KEY_PPAGE:
        eds[focus].cursorpos -= (eds[focus].lines - 2) * 16;
        break;
    case KEY_RIGHT:
        eds[focus].cursorpos++;
        break;
    case KEY_LEFT:
        eds[focus].cursorpos--;
        break;
    case KEY_UP:
        eds[focus].cursorpos-=16;
        break;
    case KEY_DOWN:
        eds[focus].cursorpos+=16;
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        insert_value = ch - '0';
        break;
    case 'a':
    case 'A':
        insert_value = 0xa;
        break;
    case 'b':
    case 'B':
        insert_value = 0xb;
        break;
    case 'c':
    case 'C':
        insert_value = 0xc;
        break;
    case 'd':
    case 'D':
        insert_value = 0xd;
        break;
    case 'e':
    case 'E':
        insert_value = 0xe;
        break;
    case 'f':
    case 'F':
        insert_value = 0xf;
        break;
    }

    if (insert_value != -1)
    {
        if (eds[focus].cursorpos & 1)
            eds[focus].memarea[eds[focus].memoffset + (eds[focus].cursorpos / 2)] = (eds[focus].memarea[eds[focus].memoffset + (eds[focus].cursorpos / 2)] & 0xf0) | insert_value;
        else
            eds[focus].memarea[eds[focus].memoffset + (eds[focus].cursorpos / 2)] = (eds[focus].memarea[eds[focus].memoffset + (eds[focus].cursorpos / 2)] & 0x0f) | (insert_value << 4);
        eds[focus].cursorpos++;
    }

    while (eds[focus].cursorpos < 0)
    {
        eds[focus].memoffset -= 8;
        if (eds[focus].memoffset < 0)
        {
            eds[focus].memoffset = 0;
            eds[focus].cursorpos = 0;
        }
        else
        {
            eds[focus].cursorpos += 16;
        }
    }
    while (eds[focus].cursorpos > (eds[focus].lines - 2) * 16 - 1)
    {
        eds[focus].memoffset += 8;
        if (eds[focus].memoffset > (eds[focus].maxmem - (eds[focus].lines - 2)*8))
        {
            eds[focus].memoffset = (eds[focus].maxmem - (eds[focus].lines - 2)*8);
            eds[focus].cursorpos = (eds[focus].lines - 2) * 16 - 1;
        }
        else
        {
            eds[focus].cursorpos -= 16;
        }
    }
}

#define MASK_PRINTABLES(x) (((x) > 31)?(((x) < 127)?(x):'.'):'.')

void memeditor_update(struct em8051 *aCPU)
{
    int i, j, bytevalue;
    for (i = 0; i < 5; i++)
    {
        werase(eds[i].view);
        if (eds[i].memarea)
        {
            for (j = 0; j < eds[i].lines - 2; j++)
            {
                wprintw(eds[i].view,"%04X %02X %02X %02X %02X %02X %02X %02X %02X %c%c%c%c%c%c%c%c\n", 
                    j*8+eds[i].memoffset+eds[i].memviewoffset, 
                    eds[i].memarea[j*8+0+eds[i].memoffset], 
                    eds[i].memarea[j*8+1+eds[i].memoffset], 
                    eds[i].memarea[j*8+2+eds[i].memoffset], 
                    eds[i].memarea[j*8+3+eds[i].memoffset],
                    eds[i].memarea[j*8+4+eds[i].memoffset], 
                    eds[i].memarea[j*8+5+eds[i].memoffset], 
                    eds[i].memarea[j*8+6+eds[i].memoffset], 
                    eds[i].memarea[j*8+7+eds[i].memoffset],
                    MASK_PRINTABLES(eds[i].memarea[j*8+0+eds[i].memoffset]), 
                    MASK_PRINTABLES(eds[i].memarea[j*8+1+eds[i].memoffset]), 
                    MASK_PRINTABLES(eds[i].memarea[j*8+2+eds[i].memoffset]), 
                    MASK_PRINTABLES(eds[i].memarea[j*8+3+eds[i].memoffset]),
                    MASK_PRINTABLES(eds[i].memarea[j*8+4+eds[i].memoffset]), 
                    MASK_PRINTABLES(eds[i].memarea[j*8+5+eds[i].memoffset]), 
                    MASK_PRINTABLES(eds[i].memarea[j*8+6+eds[i].memoffset]), 
                    MASK_PRINTABLES(eds[i].memarea[j*8+7+eds[i].memoffset]));
            }
        }
    }

    bytevalue = eds[focus].memarea[eds[focus].cursorpos / 2 + eds[focus].memoffset];
    wattron(eds[focus].view, A_REVERSE);
    wmove(eds[focus].view, eds[focus].cursorpos / 16, 5 + ((eds[focus].cursorpos % 16) / 2) * 3 + (eds[focus].cursorpos & 1));
    wprintw(eds[focus].view,"%X", (bytevalue >> (4 * (!(eds[focus].cursorpos & 1)))) & 0xf);
    wattroff(eds[focus].view, A_REVERSE);

    for (i = 0; i < 5; i++)
        wrefresh(eds[i].view);

    refresh();
}
