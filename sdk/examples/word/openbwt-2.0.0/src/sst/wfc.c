/*
 * wfc.c for the OpenBWT project
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


/*--- Weighted Frequency Count ---*/

int
obwt_wfc_encode(const unsigned char *T, unsigned char *U, int n,
                int numlevels, const int *Levels, const int *W) {
  int C[256];
  unsigned char S2R[256], R2S[256];
  unsigned char *buffer;
  int i, j, k, t, front, windowsize, buffersize;
  int a;
  unsigned char c, r, old_c, lastc;

  windowsize = Levels[numlevels - 1];
  buffer = (unsigned char *)malloc(windowsize * sizeof(unsigned char)); if(buffer == NULL) { return -1; }
  for(a = 0; a < 256; ++a) { S2R[a] = R2S[a] = (unsigned char)a; C[a] = 0; }
  for(i = 0; i < windowsize; ++i) { buffer[i] = 0; }

  for(i = 0, front = 0, buffersize = 0, lastc = 0; i < n; ++i) {
    if((c = T[i]) == lastc) { U[i] = 0; }
    else {
      U[i] = r = S2R[c];

      t = (C[c] += W[0]);
      for(; (0 < r) && (C[R2S[r - 1]] <= t); --r) { S2R[R2S[r] = R2S[r - 1]] = r; }
      S2R[R2S[r] = c] = r;

      for(j = 0; j < numlevels; ++j) {
        k = front - Levels[j];
        if(k < 0) { k += windowsize; }
        if(buffersize <= k) { break; }
        old_c = buffer[k];
        r = S2R[old_c];
        t = (C[old_c] -= (W[j] - W[j + 1]));
        for(; (r < 255) && (t < C[R2S[r + 1]]); ++r) { S2R[R2S[r] = R2S[r + 1]] = r; }
        S2R[R2S[r] = old_c] = r;
      }
      buffer[front++] = c;
      buffersize++;
      if(windowsize <= front) { front = 0; }
      lastc = R2S[0];
    }
  }
  free(buffer);
  return 0;
}

int
obwt_wfc_decode(const unsigned char *T, unsigned char *U, int n,
                int numlevels, const int *Levels, const int *W) {
  int C[256];
  unsigned char S2R[256], R2S[256];
  unsigned char *buffer;
  int i, j, k, t, front, windowsize, buffersize;
  int a;
  unsigned char c, r, old_c, lastc;

  windowsize = Levels[numlevels - 1];
  buffer = (unsigned char *)malloc(windowsize * sizeof(unsigned char)); if(buffer == NULL) { return -1; }
  for(a = 0; a < 256; ++a) { S2R[a] = R2S[a] = (unsigned char)a; C[a] = 0; }
  for(i = 0; i < windowsize; ++i) { buffer[i] = 0; }

  for(i = 0, front = 0, buffersize = 0, lastc = 0; i < n; ++i) {
    if((r = T[i]) == 0) { U[i] = lastc; }
    else {
      U[i] = c = R2S[r];

      t = (C[c] += W[0]);
      for(; (0 < r) && (C[R2S[r - 1]] <= t); --r) { S2R[R2S[r] = R2S[r - 1]] = r; }
      S2R[R2S[r] = c] = r;

      for(j = 0; j < numlevels; ++j) {
        k = front - Levels[j];
        if(k < 0) { k += windowsize; }
        if(buffersize <= k) { break; }
        old_c = buffer[k];
        r = S2R[old_c];
        t = (C[old_c] -= (W[j] - W[j + 1]));
        for(; (r < 255) && (t < C[R2S[r + 1]]); ++r) { S2R[R2S[r] = R2S[r + 1]] = r; }
        S2R[R2S[r] = old_c] = r;
      }
      buffer[front++] = c;
      buffersize++;
      if(windowsize <= front) { front = 0; }
      lastc = R2S[0];
    }
  }
  free(buffer);
  return 0;
}


int
obwt_awfc_calculate_s(const unsigned char *T, int n) {
  int C[256];
  int i, j, k, avg;
  int a;
  unsigned char c;

  for(a = 0; a < 256; ++a) { C[a] = 0; }
  for(i = 0; i < n; i = j) {
    for(c = T[i], j = i + 1; (j < n) && (T[j] == c); ++j) { }
    ++C[c];
  }
  for(a = 0, k = 0, i = 0; a < 256; ++a) {
    if(0 < C[a]) { i += C[a]; ++k; }
  }
  for(a = 0, avg = i * 2 / k, i = 0; a < 256; ++a) { if(avg < C[a]) { ++i; } }
  return i * 100 / k;
}

void
obwt_awfc_calculate_W(int s, int p0, int p1, int numlevels, int *W) {
  int i;
  for(i = 2; i < numlevels; ++i) {
    W[i] = (W[i - 1] * p0) / (p1 + (i * s * s));
  }
}
