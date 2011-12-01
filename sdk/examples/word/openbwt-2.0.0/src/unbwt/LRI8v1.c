/*
 * LRI8v1.c for the OpenBWT project
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
  LR-I algorithm v1 (b=256)
    space: 2.015625n+4*257+4*256 bytes
     time: O(nlg(n/b))
*/

int
obwt_unbwt_LRI8v1(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[257], D[256];
  int *S, *LS;
  unsigned short *R;
  int i, p, t, len, half;
  int c;
  unsigned short r;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 2.015625n bytes of memory. */
  R = (unsigned short *)malloc(n * sizeof(unsigned short));
  S = (int *)malloc(n / 256 * sizeof(int));
  if((R == NULL) || (S == NULL)) { free(R); free(S); return -2; }

  /* Inverse BW-transform. */
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
  for(c = 0, i = 0; c < 256; ++c) { t = D[c]; D[c] = i; i += t; }
  for(i = n - 1, t = 0; 0 <= i; --i) {
    r = R[t];
    c = r >> 8;
    U[i] = (unsigned char)c;
    LS = S + C[c];
    for(p = 0, len = C[c + 1] - C[c], half = len >> 1;
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

  return 0;
}

int
obwt_unbwt_LRI8v1_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[257], D[256];
  int *S;
  unsigned short *R;
  int i, p, s, t, pidx;
  int c;
  unsigned short r;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((numthreads <= 1) || (ISAs->numsamples == 1)) { return obwt_unbwt_LRI8v1(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 2.015625n bytes of memory. */
  R = (unsigned short *)malloc(n * sizeof(unsigned short));
  S = (int *)malloc(n / 256 * sizeof(int));
  if((R == NULL) || (S == NULL)) { free(R); free(S); return -2; }

  /* Inverse BW-transform. */
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
  for(c = 0, i = 0; c < 256; ++c) { t = D[c]; D[c] = i; i += t; }
# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, C, D, S, R, pidx) private(i, p, s, t, c, r) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int *LS;
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
      U[i] = (unsigned char)c;
      LS = S + C[c];
      for(p = 0, len = C[c + 1] - C[c], half = len >> 1;
          0 < len;
          len = half, half >>= 1) {
        if(LS[p + half] <= t) { p += half + 1; half -= (len & 1) ^ 1; }
      }
      t = (p << 8) + (r & 0xff) + D[c];
      if(t < pidx) { ++t; }
    }
  }

  /* Deallocate memory. */
  free(R);
  free(S);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_LRI8v1(T, U, n, ISAs->samples[0]);
#endif
}
