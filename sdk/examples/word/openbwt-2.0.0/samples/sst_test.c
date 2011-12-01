/*
 * sst_test.c for the OpenBWT project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "openbwt.h"
#include "config.h"
#include "lfs.h"


static const int lg_table[256]= {
 -1,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static
int
ilg(int n) {
  return (n & 0xffff0000) ?
          ((n & 0xff000000) ?
            24 + lg_table[(n >> 24) & 0xff] :
            16 + lg_table[(n >> 16) & 0xff]) :
          ((n & 0x0000ff00) ?
             8 + lg_table[(n >>  8) & 0xff] :
             0 + lg_table[(n >>  0) & 0xff]);
}

static
int
rleexp(const unsigned char *T, unsigned char *U, int n, int min, int *outbits) {
  int i, j, k, m, bits, run;
  for(i = 0, m = 0, k = 0, bits = 0; i < n; i = j) {
    for(j = i + 1; (j < n) && (T[i] == T[j]); ++j) { }
    run = j - i;
    if(min <= run) {
      bits += ilg(run - min + 1);
      run = min + ilg(run - min + 1);
    }
    k = m + run;
    for(; m < k; ++m) { U[m] = T[i]; }
  }
  *outbits = bits;
  return m;
}

typedef int (*encoder_func)(const unsigned char *T, const int *A, int n, int cs);

static
double
getsize_and_update(int *counts, int num, int c, int incr, int threshold) {
  double bits = -log((double)(counts[c] + 1) / (double)(counts[num] + num));
  int i;
  counts[c] += incr;
  counts[num] += incr;
  if(threshold < counts[num]) {
    for(i = 0, counts[num] = 0; i < num; ++i) { counts[num] += counts[i] >>= 1; }
  }
  return bits / log(2.0);
}

static
int
test_encoder1(const unsigned char *T, const int *A, int n, int cs) {
  /* Simple Structured Coding Model */
  double bits = 0.0;
  int C0[11];
  int C1[265];
  int i, e, f, l, r, c;
  if(A != NULL) {
    for(i = 0; i < 256; ++i) { bits += ilg(A[i] + 1) * 2 + 1; }
  }
  for(i = 0; i <  11; ++i) { C0[i] = 0; }
  for(i = 0; i < 265; ++i) { C1[i] = 0; }
  for(i = 0; i < n; ++i) {
    c = (cs == 1) ? T[i] : ((const int *)T)[i];
    e = ilg(c) + 1;
    f = (e <= 8) ? e : 9;
    bits += getsize_and_update(C0, 10, f, 128, 1 << 14);
    if(e <= 8) {
      r = 1 << e, l = r >> 1;
      bits += getsize_and_update(&(C1[l + e]), r - l, c - l, 16, 1 << 12);
    } else {
      bits += e - 9 + 1;
      bits += e - 1;
    }
  }
  return (int)ceil(bits / 8.0);
}

int
test_encoder2(const unsigned char *T, const int *A, int n, int cs) {
  /* RLE0 + Simple Structured Coding Model */
  double bits = 0.0;
  int C0[11];
  int C1[265];
  int C2[3];
  int i, e, f, l, r, c, run;
  if(A != NULL) {
    for(i = 0; i < 256; ++i) { bits += ilg(A[i] + 1) * 2 + 1; }
  }
  for(i = 0; i <  11; ++i) { C0[i] = 0; }
  for(i = 0; i < 265; ++i) { C1[i] = 0; }
  for(i = 0; i <   3; ++i) { C2[i] = 0; }
  for(i = 0, run = 0; i < n; ++i) {
    c = (cs == 1) ? T[i] : ((const int *)T)[i];
    run += 1;
    if(c != 0) {
      e = ilg(run) - 1;
      for(; 0 <= e; --e) {
        bits += getsize_and_update(C0, 10, 0, 128, 1 << 14);
        bits += getsize_and_update(C2, 2, (run >> e) & 1, 32, 1 << 14);
      }
      run = 0;

      e = ilg(c) + 1;
      f = (e <= 8) ? e : 9;
      bits += getsize_and_update(C0, 10, f, 128, 1 << 14);
      if(e <= 8) {
        r = 1 << e, l = r >> 1;
        bits += getsize_and_update(&(C1[l + e]), r - l, c - l, 16, 1 << 12);
      } else {
        bits += e - 9 + 1; /* putunary(e - 9) */
        bits += e - 1;     /* putbits(e - 1, c) */
      }
    }
  }
  e = ilg(++run) - 1;
  for(; 0 <= e; --e) {
    bits += getsize_and_update(C0, 10, 0, 128, 1 << 14);
    bits += getsize_and_update(C2, 2, (run >> e) & 1, 32, 1 << 14);
  }
  bits += getsize_and_update(C0, 10, 1, 128, 1 << 14);

  return (int)ceil(bits / 8.0);
}

void
test_SSTs(const unsigned char *T, int n, int orign, int extrabytes, encoder_func func) {
  double etime, dtime;
  unsigned char *U8, *V;
  int *U32;
  int A[256];
  int i, m, size;
  clock_t start, finish;
  U32 = (int *)malloc(n * sizeof(int));
  U8 = (unsigned char *)malloc(n * sizeof(unsigned char));
  V = (unsigned char *)malloc(n * sizeof(unsigned char));

  fprintf(stderr, "  SST       Encoding Time   Decoding Time   Compressed Size\n");

  /* Move To Front */
  start = clock();
  obwt_mtf_encode(T, U8, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_mtf_decode(U8, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  MTF         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Move One From Front */
  start = clock();
  obwt_m1ff_encode(T, U8, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_m1ff_decode(U8, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  M1FF        %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Move One From Front 2 */
  start = clock();
  obwt_m1ff2_encode(T, U8, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_m1ff2_decode(U8, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  M1FF2       %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Sorted Inversion Frequencies */
  start = clock();
  m = obwt_inversioncoder_encode(T, U32, A, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_inversioncoder_decode(U32, A, V, n, m);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)((unsigned char *)U32, A, m, 4) + extrabytes;
  fprintf(stderr, "  SIF         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Distance Coding */
  start = clock();
  m = obwt_distancecoder_encode(T, U32, A, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_distancecoder_decode(U32, A, V, n, m);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  for(i = 0; i < 256; ++i) { A[i] += 1; }
  size = (*func)((unsigned char *)U32, A, m, 4) + extrabytes;
  fprintf(stderr, "  DC          %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Sorted Distance Coding */
  start = clock();
  m = obwt_sdcoder_encode(T, U32, A, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_sdcoder_decode(U32, A, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)((unsigned char *)U32, A, m, 4) + extrabytes;
  fprintf(stderr, "  SDC         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Sorted Rank Coding */
  start = clock();
  obwt_srcoder_encode(T, U8, A, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_srcoder_decode(U8, A, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, A, n, 1) + extrabytes;
  fprintf(stderr, "  SRC         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Transpose */
  start = clock();
  obwt_transpose_encode(T, U8, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_transpose_decode(U8, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  TP          %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* TimeStamp(0) (Best 2 of 3) */
  start = clock();
  obwt_timestamp0_encode(T, U8, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_timestamp0_decode(U8, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  TS0         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Best 3 of 5 */
  start = clock();
  obwt_Bx_encode(T, U8, n, 3);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_Bx_decode(U8, V, n, 3);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  Bx(3)       %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Best 4 of 7 */
  start = clock();
  obwt_Bx_encode(T, U8, n, 4);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_Bx_decode(U8, V, n, 4);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  Bx(4)       %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Best 5 of 9 */
  start = clock();
  obwt_Bx_encode(T, U8, n, 5);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_Bx_decode(U8, V, n, 5);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  Bx(5)       %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Best 6 of 11 */
  start = clock();
  obwt_Bx_encode(T, U8, n, 6);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_Bx_decode(U8, V, n, 6);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  Bx(6)       %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Sort-By-Rank(0.5) */
  start = clock();
  obwt_sortbyrank_encode(T, U8, n);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_sortbyrank_decode(U8, V, n);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  SBR         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Frequency Count */
  start = clock();
  obwt_frequencycount_encode(T, U8, n, 16, 256);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_frequencycount_decode(U8, V, n, 16, 256);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  FC          %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  /* Weighted Frequency Count */
  {
    int numlevels = 12;
    int Levels[12] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    int W[13] = {65536, 8192, 4096, 2048, 1024, 512, 256, 64, 32, 8, 4, 1, 0};
    start = clock();
    obwt_wfc_encode(T, U8, n, numlevels, Levels, W);
    finish = clock();
    etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
    start = clock();
    obwt_wfc_decode(U8, V, n, numlevels, Levels, W);
    finish = clock();
    dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
    size = (*func)(U8, NULL, n, 1) + extrabytes;
    fprintf(stderr, "  WFC         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
    if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }
  }

  /* Advanced Weighted Frequency Count */
  {
    int numlevels = 12;
    int Levels[12] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    int W[13] = {1 << 17, 1 << 14};
    int p0 = 2600;
    int p1 = 4185;
    int s;
    W[12] = 0;
    start = clock();
    s = obwt_awfc_calculate_s(T, n);
    obwt_awfc_calculate_W(s, p0, p1, numlevels, W);
    obwt_wfc_encode(T, U8, n, numlevels, Levels, W);
    finish = clock();
    etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
    start = clock();
    obwt_wfc_decode(U8, V, n, numlevels, Levels, W);
    finish = clock();
    dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
    size = (*func)(U8, NULL, n, 1) + extrabytes + 1;
    fprintf(stderr, "  AWFC        %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
    if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }
  }

  /* Incremental Frequency Count */
  start = clock();
  obwt_ifc_encode(T, U8, n, 16, 256, 8);
  finish = clock();
  etime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  start = clock();
  obwt_ifc_decode(U8, V, n, 16, 256, 8);
  finish = clock();
  dtime = (double)(finish - start) / (double)CLOCKS_PER_SEC;
  size = (*func)(U8, NULL, n, 1) + extrabytes;
  fprintf(stderr, "  IFC         %11.2f     %11.2f     %13d (%.2f%%)\n", etime, dtime, size, (double)size / (double)orign * 100.0);
  if(memcmp(T, V, n * sizeof(unsigned char)) != 0) { fprintf(stderr, "  Error...\n"); exit(1); }

  free(U32); free(U8); free(V);
}


int
main(int argc, const char *argv[]) {
  unsigned char *T, *U;
  int *A;
  FILE *fp;
  int n, m;
  clock_t start, finish;

  /* Check arguments. */
  if(argc != 2) {
    fprintf(stderr, "usage: %s FILE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Open a file. */
#if defined(HAVE_FOPEN_S)
  if(fopen_s(&fp, argv[1], "rb") != 0) { perror(argv[1]); exit(EXIT_FAILURE); }
#else
  if((fp = LFS_FOPEN(argv[1], "rb")) == NULL) { perror(argv[1]); exit(EXIT_FAILURE); }
#endif

  /* Get the file size. */
  fseek(fp, 0L, SEEK_END);
  n = (int)ftell(fp);
  rewind(fp);

  /* Allocate memory. */
  T = (unsigned char *)malloc(n * sizeof(unsigned char));
  U = (unsigned char *)malloc(n * sizeof(unsigned char));
  A = (int *)malloc(n * sizeof(int));
  if((T == NULL) || (U == NULL) || (A == NULL)) { perror(NULL); exit(EXIT_FAILURE); }

  /* Read data and close file. */
  fread(T, sizeof(unsigned char), n, fp);
  fclose(fp);
  fprintf(stderr, "Input: %s, %d bytes\n", argv[1], n);

  /** BWT **/
  fprintf(stderr, "BWT ... ");
  start = clock();
  if((m = obwt_bwt(T, U, A, n)) < 0) { fprintf(stderr, "%s: Could not allocate memory.\n", argv[0]); exit(EXIT_FAILURE); }
  finish = clock();
  fprintf(stderr, "Done. (%5.2f sec)\n", (double)(finish - start) / (double)CLOCKS_PER_SEC);

  /* Test second stage transforms. */
/*
  fprintf(stderr, "BWT -> SST -> Simple Structured Coding Model\n");
  test_SSTs(U, n, n, 12, test_encoder1);
*/
  fprintf(stderr, "BWT -> SST -> RLE0 + Simple Structured Coding Model\n");
  test_SSTs(U, n, n, 12, test_encoder2);

/*
  int extrabits;
  m = rleexp(U, U, n, 2, &extrabits);

  fprintf(stderr, "BWT -> RLE-EXP(2) -> SST -> Simple Structured Coding Model\n");
  test_SSTs(U, m, n, 12 + (extrabits + 7) / 8, test_encoder1);

  fprintf(stderr, "BWT -> RLE-EXP(2) -> SST -> RLE0 + Simple Structured Coding Model\n");
  test_SSTs(U, m, n, 12 + (extrabits + 7) / 8, test_encoder2);
*/

  /** BWTS **/
  fprintf(stderr, "BWTS ... ");
  start = clock();
  obwt_bwts(T, U, A, n);
  finish = clock();
  fprintf(stderr, "Done. (%5.2f sec)\n", (double)(finish - start) / (double)CLOCKS_PER_SEC);

  fprintf(stderr, "BWTS -> SST -> RLE0 + Simple Structured Coding Model\n");
  test_SSTs(U, n, n, 8, test_encoder2);

  /* Deallocate memory. */
  free(A);
  free(T);
  free(U);

  return 0;
}
