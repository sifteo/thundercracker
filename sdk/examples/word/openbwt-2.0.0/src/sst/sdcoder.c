/*
 * sdcoder.c for the OpenBWT project
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


/*--- Sorted Distance Coding ---*/

static
int
sdcoder_preprocess(const int *C, unsigned char *A) {
  int a, b, c, h;
  unsigned char t;
  for(a = 0, c = 0; a < 256; ++a) { if(0 < C[a]) { A[c++] = (unsigned char)a; } }
  for(h = 4; h < c; h = h * 3 + 1) { }
  do {
    h /= 3;
    for(a = h; a < c; ++a) {
      t = A[a];
      for(b = a - h;
          (0 <= b) && ((C[A[b]] < C[t]) || ((C[t] == C[A[b]]) && (t < A[b])));
          b -= h) {
        A[b + h] = A[b];
      }
      A[b + h] = t;
    }
  } while(h != 1);
  return c;
}

int
obwt_sdcoder_encode(const unsigned char *T, int *DCV, int *C, int n) {
  int P[256], L[256];
  unsigned char A[256], S2R[256], R2S[256];
  int i, j, m;
  int a, b;
  unsigned char r, c;

  /* find first symbols and count occurrences */
  for(a = 0; a < 256; ++a) { C[a] = 0; }
  for(i = 0, b = 0; i < n; i = j) {
    for(c = T[i], j = i + 1; (j < n) && (T[j] == c); ++j) { }
    if(C[c]++ == 0) { L[c] = -b; S2R[R2S[b] = c] = (unsigned char)b; b++; }
  }

  /* init arrays */
  b = sdcoder_preprocess(C, A);
  for(a = 0, m = 0; a < b; ++a) {
    c = A[a];
    P[c] = m;
    m += C[c];
  }

  /* encoding */
  for(i = 0; i < n; i = j) {
    c = T[i];
    r = S2R[c];
    DCV[P[c]++] = i - r - L[c];
    if(0 < r) {
      do { S2R[R2S[r] = R2S[r - 1]] = r; } while(0 < --r);
      S2R[R2S[0] = c] = 0;
    }

    for(j = i + 1; (j < n) && (T[j] == c); ++j) { }
    L[c] = j;
  }

  return m;
}

void
obwt_sdcoder_decode(const int *DCV, const int *C, unsigned char *T, int n) {
  int L[256], P[256], Q[256];
  unsigned char A[256];
  int i, p;
  int a, b, d, h;
  unsigned char c;

  /* init arrays and set first symbols */
  d = sdcoder_preprocess(C, A);
  for(a = 0, i = 0; a < d; ++a) {
    c = A[a];
    P[c] = i + 1;
    T[p = DCV[i]] = c;
    L[a] = (p != 0) ? p : n;
    i += C[c];
    Q[c] = i;
  }

  /* sort sym positions */
  for(h = 4; h < d; h = h * 3 + 1) { }
  do {
    h /= 3;
    for(a = h; a < d; ++a) {
      p = L[a];
      for(b = a - h; (0 <= b) && (p < L[b]); b -= h) { L[b + h] = L[b]; }
      L[b + h] = p;
    }
  } while(h != 1);

  /* decoding */
  for(i = 1; i < n; ++i) {
    for(c = T[i - 1], p = L[0]; i < p; ++i) { T[i] = c; }
    if(P[c] < Q[c]) {
      for(a = 1, p += DCV[P[c]++] + 1; L[a] <= p; ++a, ++p) { L[a - 1] = L[a]; }
      T[L[a - 1] = p] = c;
    } else {
      for(a = 1; a < d; ++a) { L[a - 1] = L[a]; }
      --d;
    }
  }
}
