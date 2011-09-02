/*-------------------------------------------------------------------------
   stdio.h - ANSI functions forward declarations

   Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net

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

#ifndef __SDC51_STDIO_H
#define __SDC51_STDIO_H 1

#include <stdarg.h>

#ifdef __ds390
#include <tinibios.h>
#endif

#include <sdcc-lib.h>

#ifndef NULL
  #define NULL (void *)0
#endif

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
  typedef unsigned int size_t;
#endif

typedef void (*pfn_outputchar)(char c, void* p) _REENTRANT;

extern int _print_format (pfn_outputchar pfn, void* pvoid, const char *format, va_list ap);

/*-----------------------------------------------------------------------*/

extern void printf_small (char *,...) _REENTRANT;
extern int printf (const char *,...);
extern int vprintf (const char *, va_list);
extern int sprintf (char *, const char *, ...);
extern int vsprintf (char *, const char *, va_list);
extern int puts(const char *);
extern char *gets(char *);
extern char getchar(void);
extern void putchar(char);

#if defined(SDCC_mcs51) && !defined(SDCC_USE_XSTACK)
extern void printf_fast(__code const char *fmt, ...) _REENTRANT;
extern void printf_fast_f(__code const char *fmt, ...) _REENTRANT;
extern void printf_tiny(__code const char *fmt, ...) _REENTRANT;
#endif

#endif /* __SDC51_STDIO_H */
