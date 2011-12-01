/*
 * distancecoder.c for the OpenBWT project
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


/*--- Distance Coder ---*/

int
obwt_distancecoder_encode(const unsigned char *T, int *DCV, int *P, int n) {
  int L[256], Q[256];
  unsigned char S2R[256], R2S[256];
  int i, j, m;
  int a;
  unsigned char c, r;

  for(a = 0; a < 256; ++a) { P[a] = -1; }
  for(i = 0, a = 0, m = 0; i < n; i = j, ++m) {
    if(0 <= P[c = T[i]]) { r = S2R[c]; DCV[Q[c]] = i - r - L[c]; /* #1 */ }
    else { r = (unsigned char)a++; P[c] = i; }
    if(0 < r) { do { S2R[R2S[r] = R2S[r - 1]] = r; } while(0 < --r); }
    S2R[R2S[0] = c] = 0;
    for(j = i + 1; (j < n) && (T[j] == c); ++j) { } /* #2 free rle */
    Q[c] = m;
    L[c] = j - 1;
  }

  for(a = a - 1; 0 <= a; --a) {
    c = R2S[a];
    if((n - a - L[c]) == 1) { break; } /* #3 */
    DCV[Q[c]] = 0;
  }

  return m - (a + 1);
}

void
obwt_distancecoder_decode(const int *DCV, const int *P, unsigned char *T, int n, int m) {
  int L[256];
  int i, j, p, t;
  int a, b, d, h;
  unsigned char c;

  /* set first symbols */
  for(a = 0, d = 0; a < 256; ++a) {
    if(0 <= (p = P[a])) { T[p] = (unsigned char)a; L[d++] = (p != 0) ? p : n; }
  }

  /* sort sym positions */
  for(h = 4; h < d; h = h * 3 + 1) { }
  do {
    h /= 3;
    for(a = h; a < d; ++a) {
      t = L[a];
      for(b = a - h; (0 <= b) && (t < L[b]); b -= h) { L[b + h] = L[b]; }
      L[b + h] = t;
    }
  } while(h != 1);

  /* decoding */
  for(i = 1, j = 0; j < m; ++i, ++j) {
    for(c = T[i - 1], p = L[0]; i < p; ++i) { T[i] = c; }
    if((t = DCV[j]) != 0) {
      for(a = 1, p += t; L[a] <= p; ++a, ++p) { L[a - 1] = L[a]; }
      T[L[a - 1] = p] = c;
    } else {
      for(a = 1; a < d; ++a) { L[a - 1] = L[a]; }
      --d;
    }
  }
  for(c = T[i - 1], p = L[0]; i < p; ++i) { T[i] = c; }
}
