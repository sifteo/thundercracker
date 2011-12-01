/*
 * srcoder.c for the OpenBWT project
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


/*--- Sorted Rank Coding ---*/

static
int
srcoder_preprocess(const int *C, unsigned char *A) {
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

void
obwt_srcoder_encode(const unsigned char *T, unsigned char *RCV, int *C, int n) {
  int P[256];
  unsigned char A[256], S2R[256], R2S[256];
  int i, j, p;
  int a, b;
  unsigned char c, r;

  /* find first symbols and count occurrences */
  for(a = 0; a < 256; ++a) { C[a] = 0; }
  for(i = 0, b = 0; i < n; i = j) {
    for(c = T[i], j = i + 1; (j < n) && (T[j] == c); ++j) { }
    if(C[c] == 0) { S2R[R2S[b] = c] = (unsigned char)b; b++; }
    C[c] += j - i;
  }

  /* init arrays */
  b = srcoder_preprocess(C, A);
  for(a = 0, i = 0; a < b; ++a) {
    c = A[a];
    P[c] = i;
    i += C[c];
  }

  /* encoding */
  for(i = 0; i < n; i = j) {
    c = T[i], r = S2R[c], p = P[c];
    RCV[p++] = r;
    if(0 < r) {
      do { S2R[R2S[r] = R2S[r - 1]] = r; } while(0 < --r);
      S2R[R2S[0] = c] = 0;
    }
    for(j = i + 1; (j < n) && (T[j] == c); ++j, ++p) { RCV[p] = 0; }
    P[c] = p;
  }
}

void
obwt_srcoder_decode(const unsigned char *RCV, const int *C, unsigned char *T, int n) {
  int P[256], Q[256];
  unsigned char A[256], R2S[256];
  int i;
  int a, b;
  unsigned char c, r, s;

  /* init arrays */
  b = srcoder_preprocess(C, A);
  for(a = 0, i = 0; a < b; ++a) {
    c = A[a];
    R2S[RCV[i]] = c;
    P[c] = i + 1;
    i += C[c];
    Q[c] = i;
  }

  /* decoding */
  for(i = 0, c = R2S[0]; i < n; ++i) {
    T[i] = c;
    if(P[c] < Q[c]) {
      if(0 < (r = RCV[P[c]++])) {
        s = 0; do { R2S[s] = R2S[s + 1]; } while(++s < r);
        R2S[r] = c;
        c = R2S[0];
      }
    } else {
      if(0 < --b) {
        s = 0; do { R2S[s] = R2S[s + 1]; } while(++s < b);
        c = R2S[0];
      }
    }
  }
}

