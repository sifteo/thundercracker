/*
 * LRI8v2.c for the OpenBWT project
 * Copyright (c) 2010 Yuta Mori. All Rights Reserved.
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
#ifdef _OPENMP
# include <omp.h>
#endif
#include "openbwt.h"

#ifdef __INTEL_COMPILER
#pragma warning(disable : 2259)
#endif


/*
  LR-I algorithm v2 (b=256)
    space: 2.015625n+4*257+4*256+4*256*(2**t) bytes
     time: O(nlg(n/b))?
*/

#define NUM_TOPBITS (8)

int
obwt_unbwt_LRI8v2(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[257], D[256];
  int *E, *LE, *S, *LS;
  unsigned short *R;
  int i, p, t, u, m, len, half, shift;
  int c;
  unsigned short r;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 2.015625n+1024*(2**NUM_TOPBITS) bytes of memory. */
  R = (unsigned short *)malloc(n * sizeof(unsigned short));
  S = (int *)malloc(n / 256 * sizeof(int));
  E = (int *)malloc((256 << NUM_TOPBITS) * sizeof(int));
  if((R == NULL) || (S == NULL) || (E == NULL)) { free(R); free(S); free(E); return -2; }

  /* Inverse BW-transform. */
  for(shift = 0; 0 < ((n - 1) >> shift); ++shift) { }
  shift = (NUM_TOPBITS <= shift) ? shift - NUM_TOPBITS : 0;
  m = (n - 1) >> shift;
  for(c = 0; c < 256; ++c) { C[c] = D[c] = 0; }
  for(i = 0; i < n; ++i) { c = T[i]; R[i] = (unsigned short)((C[c]++ & 0xff) | (c << 8)); }
  for(c = 0, i = 0; c < 256; ++c) {
    t = C[c];
    C[c] = i;
    if(0 < t) { i += (t - 1) >> 8; }
  }
  C[c] = i;
  for(i = 0; i < n; ++i) {
    c = T[i];
    t = D[c]++;
    if((t & 0xff) == 0) {
      t >>= 8;
      if(0 < t) { S[C[c] + t - 1] = i; }
    }
  }
  for(c = 0, i = 0; c < 256; ++c) {
    t = D[c];
    if(0 < t) {
      D[c] = i; i += t;
      LS = S + C[c];
      LE = E + (c << NUM_TOPBITS);
      for(u = 0, p = 0, t = C[c + 1] - C[c]; u <= m; ++u) {
        for(; (p < t) && (LS[p] <= (u << shift)); ++p) { }
        LE[u] = p;
      }
    }
  }
  for(i = n - 1, t = 0; 0 <= i; --i) {
    r = R[t];
    c = r >> 8;
    u = t >> shift;
    U[i] = (unsigned char)c;
    LS = S + C[c];
#if NUM_TOPBITS == 8
    LE = E + (r & 0xff00);
#else
    LE = E + (c << NUM_TOPBITS);
#endif
    p = LE[u];
    len = ((u < m) ? LE[u + 1] : (C[c + 1] - C[c])) - p;
    for(half = len >> 1;
        0 < len;
        len = half, half >>= 1) {
      if(LS[p + half] <= t) { p += half + 1; half -= (len & 1) ^ 1; }
    }
    t = (p << 8) + (r & 0xff) + D[c];
    if(t < pidx) { ++t; }
  }

  /* Deallocate memory. */
  free(S);
  free(R);
  free(E);

  return 0;
}

int
obwt_unbwt_LRI8v2_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[257], D[256];
  int *E, *LE, *S, *LS;
  unsigned short *R;
  int i, p, s, t, u, m, pidx, shift;
  int c;
  unsigned short r;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((numthreads <= 1) || (ISAs->numsamples == 1)) { return obwt_unbwt_LRI8v2(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 2.015625n+1024*(2**NUM_TOPBITS) bytes of memory. */
  R = (unsigned short *)malloc(n * sizeof(unsigned short));
  S = (int *)malloc(n / 256 * sizeof(int));
  E = (int *)malloc((256 << NUM_TOPBITS) * sizeof(int));
  if((R == NULL) || (S == NULL) || (E == NULL)) { free(R); free(S); free(E); return -2; }

  /* Inverse BW-transform. */
  for(shift = 0; 0 < ((n - 1) >> shift); ++shift) { }
  shift = (NUM_TOPBITS <= shift) ? shift - NUM_TOPBITS : 0;
  m = (n - 1) >> shift;
  for(c = 0; c < 256; ++c) { C[c] = D[c] = 0; }
  for(i = 0; i < n; ++i) { c = T[i]; R[i] = (unsigned short)((C[c]++ & 0xff) | (c << 8)); }
  for(c = 0, i = 0; c < 256; ++c) {
    t = C[c];
    C[c] = i;
    if(0 < t) { i += (t - 1) >> 8; }
  }
  C[c] = i;
  for(i = 0; i < n; ++i) {
    c = T[i];
    t = D[c]++;
    if((t & 0xff) == 0) {
      t >>= 8;
      if(0 < t) { S[C[c] + t - 1] = i; }
    }
  }
  for(c = 0, i = 0; c < 256; ++c) {
    t = D[c];
    if(0 < t) {
      D[c] = i; i += t;
      LS = S + C[c];
      LE = E + (c << NUM_TOPBITS);
      for(u = 0, p = 0, t = C[c + 1] - C[c]; u <= m; ++u) {
        for(; (p < t) && (LS[p] <= (u << shift)); ++p) { }
        LE[u] = p;
      }
    }
  }
# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, C, D, S, E, R, m, pidx, shift) private(LS, LE, i, p, s, t, u, c, r) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int first, last;
    int len, half;
    if(0 < s) {
      first = (s - 1) << ISAs->lgsamplerate;
      last = s << ISAs->lgsamplerate;
      t = ISAs->samples[s] - 1;
      if(t < pidx) { ++t; }
    } else {
      first = (ISAs->numsamples - s - 1) << ISAs->lgsamplerate;
      last = n;
      t = 0;
    }
    for(i = last - 1; first <= i; --i) {
      r = R[t];
      c = r >> 8;
      u = t >> shift;
      U[i] = (unsigned char)c;
      LS = S + C[c];
  #if NUM_TOPBITS == 8
      LE = E + (r & 0xff00);
  #else
      LE = E + (c << NUM_TOPBITS);
  #endif
      p = LE[u];
      len = ((u < m) ? LE[u + 1] : (C[c + 1] - C[c])) - p;
      for(half = len >> 1;
          0 < len;
          len = half, half >>= 1) {
        if(LS[p + half] <= t) { p += half + 1; half -= (len & 1) ^ 1; }
      }
      t = (p << 8) + (r & 0xff) + D[c];
      if(t < pidx) { ++t; }
    }
  }

  /* Deallocate memory. */
  free(S);
  free(R);
  free(E);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_LRI8v2(T, U, n, ISAs->samples[0]);
#endif
}
