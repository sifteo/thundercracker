/*
 * inversioncoder.c for the OpenBWT project
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


/*--- Sorted Inversion Frequencies ---*/


static
int
inversioncoder_preprocess(const int *C, unsigned char *A, int n) {
  int avg;
  int a, b, c, h, more;
  unsigned char t;

  for(a = 0, c = 0; a < 256; ++a) { if(0 < C[a]) { A[c++] = (unsigned char)a; } }
  for(a = 0, more = 0, avg = n * 2 / c; a < c; ++a) {
    if(avg < C[A[a]]) { more++; }
  }

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
  if(10 <= (more * 100 / c)) {
    for(a = 0, b = c - 1; a < b; ++a, --b) { t = A[a], A[a] = A[b], A[b] = t; }
  }
  return c;
}

int
obwt_inversioncoder_encode(const unsigned char *T, int *IFV, int *C, int n) {
  int D[256], L[256], P[256];
  unsigned char A[256], B[256];
  int i, j, p, q;
  int a, b, d;
  unsigned char c, r;

  /* count occurrences */
  for(a = 0; a < 256; ++a) { C[a] = 0; }
  for(i = 0; i < n; i = j) {
    for(c = T[i], j = i + 1; (j < n) && (T[j] == c); ++j) { }
    C[c] += j - i;
  }

  /* init arrays */
  d = inversioncoder_preprocess(C, A, n);
  for(a = 0, i = 0; a < d; ++a) {
    c = A[a];
    B[c] = (unsigned char)a;
    P[a] = i;
    D[a] = L[a] = 0;
    i += C[c];
  }

  /* encoding */
  for(i = 0; i < n; i = j) {
    c = T[i];
    r = B[c];

    if((b = d - r - 1) == 0) { q = 0; }
    else { for(q = D[0], --b; 0 < b; b &= (b - 1)) { q += D[b]; } }
    p = P[r];
    IFV[p] = q - L[r];
    L[r] = q;

    for(j = i + 1, q = p + 1; (j < n) && (T[j] == c); ++j, ++q) { IFV[q] = 0; }
    P[r] = q;
    q -= p;

    if((b = d - r - 1) == 0) { D[0] += q; }
    else { do { D[b] += q; b += b & (-b); } while(b < d); }
  }
  for(i = n - C[A[d - 1]]; (0 < i) && (IFV[i - 1] == 0); --i) { }
  return i;
}

void
obwt_inversioncoder_decode(const int *IFV, const int *C, unsigned char *T, int n, int m) {
  int D1[256], D2[256], D3[256], P[256], Q[256];
  int Gstart[256], Gsize[256];
  unsigned char A[256], B[256];
  int i, t, mindist;
  int a, b, d, g, numg;

  /* init arrays */
  d = inversioncoder_preprocess(C, A, n);
  for(a = 0, i = 0; a < d; ++a) {
    D3[a] = (i < m) ? IFV[i] : 0;
    P[a] = i + 1;
    i += C[B[a] = A[a]];
    Q[a] = i;
  }

  /* split symbols */
  if(C[A[0]] <= C[A[d - 1]]) {
    for(a = 0, numg = 0; a < d; a = b, ++numg) {
      for(i = C[A[a]], b = a + 1;
          (b < d) && ((i + C[A[b]]) <= C[A[d - 1]]);
          i += C[A[b]], ++b) { }
      Gsize[numg] = b - a;
    }
  } else {
    for(a = d - 1, numg = 0; 0 <= a; a = b, ++numg) {
      for(i = C[A[a]], b = a - 1;
          (0 <= b) && ((i + C[A[b]]) <= C[A[0]]);
          i += C[A[b]], --b) { }
      Gsize[numg] = a - b;
    }
    for(a = 0, b = numg - 1; a < b; ++a, --b) {
      g = Gsize[a], Gsize[a] = Gsize[b], Gsize[b] = g;
    }
  }
  for(a = 0, g = 0; a < d; ++g) {
    Gstart[g] = a, b = a + Gsize[g];
    for(mindist = D3[a]; ++a < b;) { if(D3[a] < mindist) { mindist = D3[a]; } }
    D1[g] = D2[g] = mindist;
  }

  /* Decoding */
  for(i = 0; i < n; ++i) {
    for(g = 0; D1[g] != 0; ++g) { --D1[g]; } /* search group */
    a = Gstart[g], b = Gsize[g];
    if(1 < b) {
      b += a;
      t = D2[g];
      for(mindist = n; D3[a] != t; ++a) { /* search symbol */
        if(--D3[a] < mindist) { mindist = D3[a]; }
      }
      T[i] = B[a];
      if(P[a] < Q[a]) {
        /* update distance */
        D3[a] += (P[a] < m) ? IFV[P[a]] : 0; P[a]++;
        if(t != mindist) {
          for(;((mindist <= D3[a]) || (t != (mindist = D3[a]))) && (++a < b);) { }
        }
      } else {
        /* delete symbol from group */
        --Gsize[g];
        for(; ++a < b;) {
          D3[a - 1] = D3[a];
          if(D3[a - 1] < mindist) { mindist = D3[a - 1]; }
          P[a - 1] = P[a];
          Q[a - 1] = Q[a];
          B[a - 1] = B[a];
        }
      }
      D1[g] = mindist - t, D2[g] = mindist;
    } else {
      T[i] = B[a];
      if(P[a] < Q[a]) { D1[g] = (P[a] < m) ? IFV[P[a]] : 0; P[a]++; }
      else { /* delete group */
        for(; ++g < numg;) {
          D1[g - 1] = D1[g];
          D2[g - 1] = D2[g];
          Gsize[g - 1]  = Gsize[g];
          Gstart[g - 1] = Gstart[g];
        }
        --numg;
      }
    }
  }
}
