/*
 * unbwt.c for the OpenBWT project
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
read_vbcode(FILE *fp, int *n) {
  /* variable-byte coding */
  unsigned char A[16];
  int i;
  unsigned char c;
  *n = 0;
  if(fread(&c, sizeof(unsigned char), 1, fp) != 1) { return 1; }
  i = c & 15;
  if(0 < i) {
    if(fread(&(A[0]), sizeof(unsigned char), i, fp) != (size_t)i) { return 1; }
    while(0 < i--) { *n = (*n << 8) | A[i]; }
  }
  *n = (*n << 4) | ((c >> 4) & 15);
  return 0;
}

typedef int(*unbwt_func_t)(const unsigned char *, unsigned char *, int, int);
typedef int(*unbwt_omp_func_t)(const obwt_ISAs_t *, const unsigned char *, unsigned char *, int, int);

struct _unbwt_funcdata_t {
  const char *name;
  unbwt_func_t func;
  unbwt_omp_func_t ompfunc;
};

static
void
print_help(const char *progname, int status, const struct _unbwt_funcdata_t *unbwt_funcdata) {
  int i;
  fprintf(stderr, "usage: %s [-t num] [-a type_name] INFILE OUTFILE\n", progname);
  fprintf(stderr, "  -t num         set number of threads to num [1..512] (default: auto)\n");
  fprintf(stderr, "  -a type_name   set unbwt algorithm (default: indexPSIv2)");
  for(i = 0; unbwt_funcdata[i].name != NULL; ++i) {
    if(i % 5 == 0) { fprintf(stderr, "\n      "); }
    else { fprintf(stderr, " "); }
    fprintf(stderr, "%-11s", unbwt_funcdata[i].name);
  }
  fprintf(stderr, "\n\n");

  exit(status);
}

int
main(int argc, const char *argv[]) {
  const struct _unbwt_funcdata_t unbwt_funcdata[22] = {
    {"basisPSI",        obwt_unbwt_basisPSI,      obwt_unbwt_basisPSI_omp},
    {"basisLF",         obwt_unbwt_basisLF,       obwt_unbwt_basisLF_omp},
    {"bw94",            obwt_unbwt_bw94,          obwt_unbwt_bw94_omp},
    {"saPSI",           obwt_unbwt_saPSI,         obwt_unbwt_saPSI_omp},
    {"saLF",            obwt_unbwt_saLF,          obwt_unbwt_saLF_omp},
    {"mergedTPSI",      obwt_unbwt_mergedTPSI,    obwt_unbwt_mergedTPSI_omp},
    {"mergedTL",        obwt_unbwt_mergedTL,      obwt_unbwt_mergedTL_omp},
    {"indexPSIv1",      obwt_unbwt_indexPSIv1,    obwt_unbwt_indexPSIv1_omp},
    {"indexPSIv2",      obwt_unbwt_indexPSIv2,    obwt_unbwt_indexPSIv2_omp},
    {"indexLFv1",       obwt_unbwt_indexLFv1,     obwt_unbwt_indexLFv1_omp},
    {"indexLFv2",       obwt_unbwt_indexLFv2,     obwt_unbwt_indexLFv2_omp},
    {"unlimTPSI",       obwt_unbwt_unlimTPSI,     obwt_unbwt_unlimTPSI_omp},
    {"unlimTLF",        obwt_unbwt_unlimTLF,      obwt_unbwt_unlimTLF_omp},
    {"LL",              obwt_unbwt_LL,            obwt_unbwt_LL_omp},
    {"LRI",             obwt_unbwt_LRI,           obwt_unbwt_LRI_omp},
    {"LRI8v1",          obwt_unbwt_LRI8v1,        obwt_unbwt_LRI8v1_omp},
    {"LRI8v2",          obwt_unbwt_LRI8v2,        obwt_unbwt_LRI8v2_omp},
    {"biPSIv1",         obwt_unbwt_biPSIv1,       obwt_unbwt_biPSIv1_omp},
    {"biPSIv2",         obwt_unbwt_biPSIv2,       obwt_unbwt_biPSIv2_omp},
    {"biPSIv1_fullomp", obwt_unbwt_biPSIv1,       obwt_unbwt_biPSIv1_fullomp},
    {"biPSIv2_fullomp", obwt_unbwt_biPSIv2,       obwt_unbwt_biPSIv2_fullomp},
    {NULL, NULL, NULL}
  };
  FILE *fp, *ofp;
  const char *fname, *ofname;
  unsigned char *T;
  obwt_ISAs_t ISAs;
  LFS_OFF_T n;
  size_t m;
#ifdef _OPENMP
  double start, finish;
#else
  clock_t start, finish;
#endif
  int i, t, pidx = 0, numsamples, blocksize, lgsrate, numthreads = 1;
  int algo = 8, err, needclose = 3;

#ifdef _OPENMP
  numthreads = omp_get_max_threads();
#endif

  /* Check arguments. */
  if((argc == 1) ||
     (strcmp(argv[1], "-h") == 0) ||
     (strcmp(argv[1], "--help") == 0)) { print_help(argv[0], EXIT_SUCCESS, unbwt_funcdata); }
  if((argc != 3) && (argc != 5) && (argc != 7)) { print_help(argv[0], EXIT_FAILURE, unbwt_funcdata); }
  i = 1;
  for(i = 1; 4 <= (argc - i); i += 2) {
    if(strcmp(argv[i], "-t") == 0) {
      numthreads = atoi(argv[i + 1]);
      if(numthreads < 1) { numthreads = 1; }
      else if(512 < numthreads) { numthreads = 512; }
    } else if(strcmp(argv[i], "-a") == 0) {
      for(algo = 0; (unbwt_funcdata[algo].name != NULL) &&
                    (strcmp(argv[i + 1], unbwt_funcdata[algo].name) != 0); ++algo) { }
      if(unbwt_funcdata[algo].name == NULL) {
        fprintf(stderr, "%s: `%s' is an unknown type_name.\n", argv[0], argv[i + 1]);
        print_help(argv[0], EXIT_FAILURE, unbwt_funcdata);
      }
    } else {
      fprintf(stderr, "%s: `%s' is an unknown option.\n", argv[0], argv[i]);
      print_help(argv[0], EXIT_FAILURE, unbwt_funcdata);
    }
  }

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

  /* Read blocksize and lgsrate. */
  if((read_vbcode(fp, &blocksize) != 0) || (read_vbcode(fp, &lgsrate) != 0)) {
    fprintf(stderr, "%s: Could not read data from `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Allocate blocksize bytes of memory. */
  T = (unsigned char *)malloc((size_t)(blocksize * sizeof(unsigned char)));
  if(T == NULL) {
    fprintf(stderr, "%s: Could not allocate memory.\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "UnBWT (blocksize %d) ... ", blocksize);
#ifdef _OPENMP
  start = omp_get_wtime();
#else
  start = clock();
#endif
  for(n = 0; read_vbcode(fp, &t) == 0; n += m) {
    assert(t <= blocksize);
    m = blocksize - t; if(m == 0) { continue; }

    /* Read the inverse suffix array samples. */
    if(1 < numthreads) {
      if(obwt_ISAs_alloc(&ISAs, (int)m, lgsrate) != 0) {
        fprintf(stderr, "%s: Could not allocate memory.\n", argv[0]);
        exit(EXIT_FAILURE);
      }
      for(i = 0; i < ISAs.numsamples; ++i) {
        if(read_vbcode(fp, &(ISAs.samples[i])) != 0) {
          fprintf(stderr, "%s: Could not read data from `%s': ", argv[0], fname);
          perror(NULL);
          exit(EXIT_FAILURE);
        }
      }
    } else {
      numsamples = ((m - 1) >> lgsrate) + 1;
      if(read_vbcode(fp, &pidx) != 0) {
        fprintf(stderr, "%s: Could not read data from `%s': ", argv[0], fname);
        perror(NULL);
        exit(EXIT_FAILURE);
      }
      for(i = 1; i < numsamples; ++i) {
        if(read_vbcode(fp, &t) != 0) {
          fprintf(stderr, "%s: Could not read data from `%s': ", argv[0], fname);
          perror(NULL);
          exit(EXIT_FAILURE);
        }
      }
    }

    /* Read m bytes of data. */
    if(fread(T, sizeof(unsigned char), m, fp) != m) {
      fprintf(stderr, "%s: %s `%s': ",
        argv[0],
        (ferror(fp) || !feof(fp)) ? "Could not read data from" : "Unexpected EOF in",
        fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }

    /* Inverse Burrows-Wheeler Transform. */
    if(1 < numthreads) {
      if((err = (*(unbwt_funcdata[algo].ompfunc))(&ISAs, T, T, (int)m, numthreads)) != 0) {
        fprintf(stderr, "%s (obwt_unbwt_%s%s): %s.\n",
          unbwt_funcdata[algo].name, (algo <= 19) ? "_omp" : "", argv[0],
          (err == -1) ? "Invalid data" :
          ((err == -2) ? "Could not allocate memory" : "Block size is too large (>=16MiB)"));
        exit(EXIT_FAILURE);
      }
      obwt_ISAs_dealloc(&ISAs);
    } else {
      if((err = (*(unbwt_funcdata[algo].func))(T, T, (int)m, pidx)) != 0) {
        fprintf(stderr, "%s (obwt_unbwt_%s): %s.\n",
          unbwt_funcdata[algo].name, argv[0],
          (err == -1) ? "Invalid data" :
          ((err == -2) ? "Could not allocate memory" : "Block size is too large (>=16MiB)"));
        exit(EXIT_FAILURE);
      }
    }

    /* Write m bytes of data. */
#ifndef DISABLE_OUTPUT
    if(fwrite(T, sizeof(unsigned char), m, ofp) != m) {
      fprintf(stderr, "%s: Could not write data to `%s': ", argv[0], ofname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
#endif
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
  free(T);

  return 0;
}
