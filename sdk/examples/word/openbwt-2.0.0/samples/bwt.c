/*
 * bwt.c for the OpenBWT project
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
#include <stdio.h>
#if defined(HAVE_STDLIB_H)
# include <stdlib.h>
#else
# if defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
#endif
#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#else
# if defined(HAVE_STDINT_H)
#  include <stdint.h>
# endif
#endif
#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif
#if defined(HAVE_STRING_H)
# include <string.h>
#else
# if defined(HAVE_STRINGS_H)
#  include <strings.h>
# endif
#endif
#if defined(HAVE_IO_H) && defined(HAVE_FCNTL_H)
# include <io.h>
# include <fcntl.h>
#endif
#include <time.h>
#ifdef _OPENMP
# include <omp.h>
#endif
#include "lfs.h"
#include "openbwt.h"


static
int
write_vbcode(FILE *fp, int n) {
  /* variable-byte coding */
  unsigned char A[16];
  int i, t = n & 15; n >>= 4;
  for(i = 1; 0 < n; ++i, n >>= 8) { A[i] = (unsigned char)(n & 0xff); }
  A[0] = (unsigned char)((i - 1) | (t << 4));
  return (fwrite(&(A[0]), sizeof(unsigned char), i, fp) != (size_t)i);
}

static
void
print_help(const char *progname, int status) {
  fprintf(stderr, "usage: %s [-b num] [-s num] INFILE OUTFILE\n", progname);
  fprintf(stderr, "  -b num    set block size to num MiB [1..512] (default: 32)\n");
  fprintf(stderr, "  -s num    set sample rate to 2**num [8..30] (default: 20)\n\n");
  exit(status);
}

int
main(int argc, const char *argv[]) {
  FILE *fp, *ofp;
  const char *fname, *ofname;
  unsigned char *T, *U;
  int *SA;
  obwt_ISAs_t ISAs;
  LFS_OFF_T n;
  size_t m;
#ifdef _OPENMP
  double start, finish;
#else
  clock_t start, finish;
#endif
  int pidx, numsamples, blocksize = 32, lgsrate = 20;
  int i, needclose = 3;

  /* Check arguments. */
  if((argc == 1) ||
     (strcmp(argv[1], "-h") == 0) ||
     (strcmp(argv[1], "--help") == 0)) { print_help(argv[0], EXIT_SUCCESS); }
  if((argc != 3) && (argc != 5) && (argc != 7)) { print_help(argv[0], EXIT_FAILURE); }
  i = 1;
  for(i = 1; 4 <= (argc - i); i += 2) {
    if(strcmp(argv[i], "-b") == 0) {
      blocksize = atoi(argv[i + 1]);
      if(blocksize < 1) { blocksize = 1; }
      else if(512 < blocksize) { blocksize = 512; }
    } else if(strcmp(argv[i], "-s") == 0) {
      lgsrate = atoi(argv[i + 1]);
      if(lgsrate < 8) { lgsrate = 8; }
      else if(30 < lgsrate) { lgsrate = 30; }
    } else { print_help(argv[0], EXIT_FAILURE); }
  }
  blocksize <<= 20;

  /* Open a file for reading. */
  if(strcmp(argv[i], "-") != 0) {
#if defined(HAVE_FOPEN_S)
    if(fopen_s(&fp, fname = argv[i], "rb") != 0) {
#else
    if((fp = LFS_FOPEN(fname = argv[i], "rb")) == NULL) {
#endif
      fprintf(stderr, "%s: Could not open file `%s' for reading: ", argv[0], fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
#if defined(HAVE__SETMODE) && defined(HAVE__FILENO)
    if(_setmode(_fileno(stdin), _O_BINARY) == -1) {
      fprintf(stderr, "%s: Could not set mode: ", argv[0]);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
#endif
    fp = stdin;
    fname = "stdin";
    needclose ^= 1;
  }
  ++i;

  /* Open a file for writing. */
  if(strcmp(argv[i], "-") != 0) {
#if defined(HAVE_FOPEN_S)
    if(fopen_s(&ofp, ofname = argv[i], "wb") != 0) {
#else
    if((ofp = LFS_FOPEN(ofname = argv[i], "wb")) == NULL) {
#endif
      fprintf(stderr, "%s: Could not open file `%s' for writing: ", argv[0], ofname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
#if defined(HAVE__SETMODE) && defined(HAVE__FILENO)
    if(_setmode(_fileno(stdout), _O_BINARY) == -1) {
      fprintf(stderr, "%s: Could not set mode: ", argv[0]);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
#endif
    ofp = stdout;
    ofname = "stdout";
    needclose ^= 2;
  }

  /* Get the file size. */
  if(LFS_FSEEK(fp, 0, SEEK_END) == 0) {
    if((n = LFS_FTELL(fp)) < 0) {
      fprintf(stderr, "%s: Could not ftell `%s': ", argv[0], fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
    if(0x20000000L < n) { n = 0x20000000L; }
    if((blocksize == 0) || (n < blocksize)) { blocksize = (int)n; }
  } else if(blocksize == 0) { blocksize = 32 << 20; }
  rewind(fp);

  /* Allocate 5*blocksize bytes of memory. */
  T = (unsigned char *)malloc((size_t)(blocksize * sizeof(unsigned char)));
  SA = (int *)malloc((size_t)(blocksize * sizeof(int)));
  if((T == NULL) || (SA == NULL)) {
    fprintf(stderr, "%s: Could not allocate memory.\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  U = (unsigned char *)SA;

  /* Write blocksize and lgsrate. */
  if((write_vbcode(ofp, blocksize) != 0) || (write_vbcode(ofp, lgsrate) != 0)) {
    fprintf(stderr, "%s: Could not write data to `%s': ", argv[0], ofname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "  BWT (blocksize %d) ... ", blocksize);
#ifdef _OPENMP
  start = omp_get_wtime();
#else
  start = clock();
#endif
  for(n = 0; 0 < (m = fread(T, sizeof(unsigned char), (size_t)blocksize, fp)); n += m) {
    /* Write the difference between the blocksize and the current size. */
    if(write_vbcode(ofp, blocksize - m) != 0) {
      fprintf(stderr, "%s: Could not write data to `%s': ", argv[0], ofname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }

    numsamples = ((m - 1) >> lgsrate) + 1;
    if(1 < numsamples) {
      /* Construct the suffix array. */
      if(obwt_sufsort(T, SA, (int)m) < 0) {
        fprintf(stderr, "%s (obwt_sufsort): Could not allocate memory.\n", argv[0]);
        exit(EXIT_FAILURE);
      }

      /* Write the inverse suffix array samples. */
      if(obwt_ISAs_alloc(&ISAs, (int)m, lgsrate) != 0) {
        fprintf(stderr, "%s (obwt_ISAs_alloc): Could not allocate memory.\n", argv[0]);
        exit(EXIT_FAILURE);
      }
      obwt_ISAs_build_from_SA(&ISAs, SA, (int)m);
      for(i = 0; i < ISAs.numsamples; ++i) {
        if(write_vbcode(ofp, ISAs.samples[i]) != 0) {
          fprintf(stderr, "%s: Could not write data to `%s': ", argv[0], ofname);
          perror(NULL);
          exit(EXIT_FAILURE);
        }
      }
      obwt_ISAs_dealloc(&ISAs);

      /* Burrows-Wheeler Transform. */
      for(i = 0; SA[i] != 0; ++i) { U[i + 1] = T[SA[i] - 1]; }
      for(++i; i < (int)m; ++i) { U[i] = T[SA[i] - 1]; }
      U[0] = T[m - 1];
    } else {
      /* Burrows-Wheeler Transform. */
      if((pidx = obwt_bwt(T, T, SA, (int)m)) < 0) {
        fprintf(stderr, "%s (obwt_bwt): Could not allocate memory.\n", argv[0]);
        exit(EXIT_FAILURE);
      }
      U = T;

      /* Write the primary index. */
      if(write_vbcode(ofp, pidx) != 0) {
        fprintf(stderr, "%s: Could not write data to `%s': ", argv[0], ofname);
        perror(NULL);
        exit(EXIT_FAILURE);
      }
    }

    /* Write the bwted data. */
    if(fwrite(U, sizeof(unsigned char), m, ofp) != m) {
      fprintf(stderr, "%s: Could not write data to `%s': ", argv[0], ofname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  }
  if(ferror(fp)) {
    fprintf(stderr, "%s: Could not read data from `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
#ifdef _OPENMP
  finish = omp_get_wtime();
  fprintf(stderr, "%" LFS_PRId " bytes: %.4f sec\n", n, finish - start);
#else
  finish = clock();
  fprintf(stderr, "%" LFS_PRId " bytes: %.4f sec\n",
    n, (double)(finish - start) / (double)CLOCKS_PER_SEC);
#endif

  /* Close files */
  if(needclose & 1) { fclose(fp); }
  if(needclose & 2) { fclose(ofp); }

  /* Deallocate memory. */
  free(SA);
  free(T);

  return 0;
}
