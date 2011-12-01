/*
 * obwt_ISAs.c for the OpenBWT project
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

#include <assert.h>
#include <stdlib.h>
#include "openbwt.h"

int
obwt_ISAs_alloc(obwt_ISAs_t *ISAs, int n, int lgsamplerate) {
  if((ISAs == NULL) || (n < 0)) { return -1; }
  if(lgsamplerate < 0) { lgsamplerate = 0; }
  if(lgsamplerate > 32) { lgsamplerate = 32; }
  ISAs->lgsamplerate = lgsamplerate;
  ISAs->numsamples = (n + (1U << lgsamplerate) - 1) >> lgsamplerate;
  ISAs->samples = (int *)malloc((size_t)(ISAs->numsamples * sizeof(int)));
  if(ISAs->samples == NULL) { return -1; }
  return 0;
}

int
obwt_ISAs_build_from_BWT(obwt_ISAs_t *ISAs, const unsigned char *T, int n, int pidx) {
  int C[256];
  int *PSI;
  int i, j, t;
  int c;
  unsigned int sratemask;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (T == NULL) || (n < 0) ||
     (pidx < 0) || (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if((PSI = (int *)malloc((size_t)(n * sizeof(int)))) == NULL) { return -2; }
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { C[T[i]] += 1; }
  for(c = 0, i = 0; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  for(i = 0; i < n; ++i) { PSI[C[T[i]]++] = i - (i < pidx); }
  sratemask = (1U << ISAs->lgsamplerate) - 1;
  for(i = 0; i < ISAs->numsamples; ++i) { ISAs->samples[i] = 0; }
  for(i = 0, j = 0, t = pidx - 1; i < n; ++i) {
    if((i & sratemask) == 0) { assert(j < ISAs->numsamples); ISAs->samples[j++] = t + 1; }
    t = PSI[t];
  }
  free(PSI);
  return 0;
}

int
obwt_ISAs_build_from_SA(obwt_ISAs_t *ISAs, const int *SA, int n) {
  int i, t;
  unsigned int sratemask;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (SA == NULL) || (n < 0)) { return -1; }
  sratemask = (1U << ISAs->lgsamplerate) - 1;
  for(i = 0; i < ISAs->numsamples; ++i) { ISAs->samples[i] = 0; }
  for(i = 0; i < n; ++i) {
    t = SA[i];
    if((t & sratemask) == 0) {
      t >>= ISAs->lgsamplerate;
      assert(0 <= t);
      assert(t < ISAs->numsamples);
      ISAs->samples[t] = i + 1;
    }
  }
  return 0;
}

void
obwt_ISAs_dealloc(obwt_ISAs_t *ISAs) {
  if(ISAs != NULL) {
    free(ISAs->samples);
    ISAs->samples = NULL;
  }
}
