/*-------------------------------------------------------------------------
   stdarg.h - ANSI macros for variable parameter list

   Copyright (C) 2000, Michael Hope

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

#ifndef __SDC51_STDARG_H
#define __SDC51_STDARG_H 1

#if defined(__z80) || defined(__gbz80) || defined(__hc08)

typedef unsigned char * va_list;
#define va_start(marker, last)  { marker = (va_list)&last + sizeof(last); }
#define va_arg(marker, type)    *((type *)((marker += sizeof(type)) - sizeof(type)))

#elif defined(__ds390) || defined(__ds400)

typedef unsigned char * va_list;
#define va_start(marker, first) { marker = (va_list)&first; }
#define va_arg(marker, type)    *((type *)(marker -= sizeof(type)))

#elif defined(SDCC_USE_XSTACK)

typedef unsigned char __pdata * va_list;
#define va_start(marker, first) { marker = (va_list)&first; }
#define va_arg(marker, type)    *((type __pdata *)(marker -= sizeof(type)))

#else

typedef unsigned char __data * va_list;
#define va_start(marker, first) { marker = (va_list)&first; }
#define va_arg(marker, type)    *((type __data * )(marker -= sizeof(type)))

#endif

#define va_end(marker) marker = (va_list) 0;

#endif
