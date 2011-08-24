/*-------------------------------------------------------------------------
   sdcc-lib.h - Top level header file for the sdcc libraries that enables
                target specific features.

   Copyright (C) 2004, Maarten Brock, sourceforge.brock@dse.nl

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2.1, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License 
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

#ifndef __SDC51_SDCC_LIB_H
#define __SDC51_SDCC_LIB_H	1

#if defined(__z80)
#include <asm/z80/features.h>

#elif defined(__gbz80)
#include <asm/gbz80/features.h>

#elif defined(__mcs51)
#include <asm/mcs51/features.h>

#elif defined(__ds390)
#include <asm/ds390/features.h>

#else
/* PENDING */
#include <asm/default/features.h>

#endif

#endif
