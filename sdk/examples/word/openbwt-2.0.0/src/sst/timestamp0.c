/*
 * timestamp0.c for the OpenBWT project
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


/*--- TimeStamp(0) ---*/

void
obwt_timestamp0_encode(const unsigned char *T, unsigned char *U, int n) {
  int P[256], Q[256];
  unsigned char S2R[256], R2S[256];
  int i, t;
  int a;
  unsigned char c, r;

  for(a = 0; a < 256; ++a) {
    S2R[a] = R2S[a] = (unsigned char)a;
    P[a] = -(a + 1), Q[a] = -(a + 1 + 256);
  }
  for(i = 0; i < n; ++i) {
    c = T[i];
    U[i] = r = S2R[c];
    Q[c] = P[c], P[c] = i;
    for(t = Q[c]; (0 < r) && (Q[R2S[r - 1]] <= t); --r) { S2R[R2S[r] = R2S[r - 1]] = r; }
    S2R[R2S[r] = c] = r;
  }
}

void
obwt_timestamp0_decode(const unsigned char *T, unsigned char *U, int n) {
  int P[256], Q[256];
  unsigned char R2S[256];
  int i, t;
  int a;
  unsigned char c, r;

  for(a = 0; a < 256; ++a) {
    R2S[a] = (unsigned char)a;
    P[a] = -(a + 1), Q[a] = -(a + 1 + 256);
  }
  for(i = 0; i < n; ++i) {
    r = T[i];
    U[i] = c = R2S[r];
    Q[c] = P[c], P[c] = i;
    for(t = Q[c]; (0 < r) && (Q[R2S[r - 1]] <= t); --r) { R2S[r] = R2S[r - 1]; }
    R2S[r] = c;
  }
}
