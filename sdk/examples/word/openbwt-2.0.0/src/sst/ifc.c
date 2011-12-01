/*
 * ifc.c for the OpenBWT project
 * Copyright (c) 2008-2010 Yuta Mori. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif
#include <assert.h>
#include <stdio.h>
#if defined(HAVE_STDLIB_H)
# include <stdlib.h>
#else
# if defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
#endif
#include "openbwt.h"


/*--- Incremental Frequency Count ---*/

void
obwt_ifc_encode(const unsigned char *T, unsigned char *U, int n,
                int dm, int threshold, int windowsize) {
  int C[256];
  unsigned char c2r[256], r2c[256];
  int i, incr, avg, diff;
  int a;
  unsigned char c, r, lastc;

  for(a = 0; a < 256; ++a) { c2r[a] = r2c[a] = (unsigned char)a; C[a] = 0; }
  for(i = 0, incr = 16, avg = 0, lastc = 0; i < n; ++i) {
    c = T[i];
    if(lastc == c) { r = 0; }
    else if((r = c2r[c]) < c2r[lastc]) { r += 1; }
    U[i] = r;

    /* Calculate difference from average rank. */
    diff = avg;
    avg  = (avg * (windowsize - 1) + r) / windowsize;
    diff = avg - diff;
    /* Calculate increment. */
    if(0 <= diff) {
      if(dm < diff) { diff = dm; }
      incr -= (incr * diff) >> 6;
    } else {
      if(diff < -dm) { diff = -dm; }
      incr += (incr * (-diff)) >> 6;
    }
    /* Check for run, increment counter. */
    if(lastc == c) { incr += incr >> 1; }
    else { lastc = c; }
    C[c] += incr;
    /* Check for rescaling. */
    if(threshold < C[c]) {
      for(a = 0; a < 256; ++a) { C[a] = (C[a] + 1) >> 1; }
      incr = (incr + 1) >> 1;
    }
    /* Sort list. */
    r = c2r[c];
    for(; (0 < r) && (C[r2c[r - 1]] <= C[c]); --r) { c2r[r2c[r] = r2c[r - 1]] = r; }
    c2r[r2c[r] = c] = r;
  }
}

void
obwt_ifc_decode(const unsigned char *T, unsigned char *U, int n,
                int dm, int threshold, int windowsize) {
  int C[256];
  unsigned char r2c[256];
  int i, incr, avg, diff;
  int a;
  unsigned char c, r, lastc, lastr;

  for(a = 0; a < 256; ++a) { r2c[a] = (unsigned char)a; C[a] = 0; }
  for(i = 0, incr = 16, avg = 0, lastc = lastr = 0; i < n; ++i) {
    r = T[i];
    if(r == 0) { c = lastc; }
    else { c = r2c[r - (r <= lastr)]; }
    U[i] = c;

    /* Calculate difference from average rank. */
    diff = avg;
    avg  = (avg * (windowsize - 1) + r) / windowsize;
    diff = avg - diff;
    /* Calculate increment. */
    if(0 <= diff) {
      if(dm < diff) { diff = dm; }
      incr -= (incr * diff) >> 6;
    } else {
      if(diff < -dm) { diff = -dm; }
      incr += (incr * (-diff)) >> 6;
    }
    /* Check for run, increment counter. */
    if(lastc == c) { incr += incr >> 1; }
    else { lastc = c; }
    C[c] += incr;
    /* Check for rescaling. */
    if(threshold < C[c]) {
      for(a = 0; a < 256; ++a) { C[a] = (C[a] + 1) >> 1; }
      incr = (incr + 1) >> 1;
    }
    /* Sort list. */
    if(r == 0) { r = lastr; }
    else if(r <= lastr) { r -= 1; }
    for(; (0 < r) && (C[r2c[r - 1]] <= C[c]); --r) { r2c[r] = r2c[r - 1]; }
    r2c[r] = c;
    lastr = r;
  }
}
