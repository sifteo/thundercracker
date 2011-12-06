#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "lzfx.h"
#include "lzf.h"
#include <errno.h>

const char* syntax = "Syntax is 'util in out c|d'\n";

typedef enum {compress, decompress} lzmode_t;
typedef unsigned char u8;
typedef int (*lzfx_fn)(const void* ibuf, unsigned int ilen,
                             void* obuf, unsigned int *olen);

int lzf_proxy_comp(const void* ibuf, unsigned int ilen,
                         void* obuf, unsigned int *olen){

    unsigned int rc;

    rc = lzf_compress(ibuf, ilen, obuf, *olen);
    if(rc==0){
        return LZFX_ESIZE;
    }

    *olen = rc;
    return 0;
}

int lzf_proxy_decomp(const void* ibuf, unsigned int ilen,
                           void* obuf, unsigned int *olen){

    unsigned int rc;

    rc = lzf_decompress(ibuf, ilen, obuf, *olen);
    if(rc==0){
        if(errno==EINVAL){
            return LZFX_ECORRUPT;
        } else if(errno == E2BIG){
            return LZFX_ESIZE;
        } else {
            fprintf(stderr, "unknown lzf fault\n");
            return -10;
        }
    }
    *olen = rc;
    return 0;
}

#define BLOCKSIZE 8192

#define GUARD_BYTES 16
#define MAGIC_VAL 42

/*  Test for buffer overrun.

    Returns:    0   No failure
                1   Failure
*/
int test_bounds(const void* ibuf, unsigned int ilen,
                lzfx_fn compressor, lzfx_fn decompressor){

    unsigned int real_length; 
    u8* comparison_buffer;
    
    u8* compressed_buffer;
    u8* plaintext_buffer;

    unsigned int compressed_length;
    unsigned int plaintext_length;
    unsigned int size_after_compression;

    int rc;
    int frc = 0;

    real_length = ((int)(ilen*1.05) == ilen ? ilen+20 : (int)ilen*1.05) + GUARD_BYTES;    
    comparison_buffer = (u8*)malloc(real_length);
    compressed_buffer = (u8*)malloc(real_length);
    plaintext_buffer = (u8*)malloc(real_length);

    memset(comparison_buffer, MAGIC_VAL, real_length);

    size_after_compression = real_length-GUARD_BYTES;

    /* Determine the actual size of the output data */
    rc = compressor(ibuf, ilen, compressed_buffer, &size_after_compression);
    if(rc<0){
        fprintf(stderr, "Failed initial compression\n");
        frc = 1;
        goto out;
    }

    memset(compressed_buffer, MAGIC_VAL, real_length);
    memset(plaintext_buffer, MAGIC_VAL, real_length);

    compressed_length = size_after_compression + GUARD_BYTES;

    rc = compressor(ibuf, ilen, compressed_buffer, &compressed_length);

    if(memcmp(comparison_buffer, compressed_buffer+size_after_compression, GUARD_BYTES)){
        fprintf(stderr, "Overrun in compressed bytes");
        frc = 1;
        goto out;
    }

    if(rc<0){
        fprintf(stderr, "Failed second compression (code %d)\n", rc);
        frc = 1;
        goto out;
    }

    plaintext_length = ilen;

    rc = decompressor(compressed_buffer, compressed_length,
                         plaintext_buffer, &plaintext_length);

    if(rc<0){
        fprintf(stderr, "Failed decompression (code %d)\n", rc);
        frc = 1;
        goto out;
    }

    if(memcmp(comparison_buffer, plaintext_buffer+plaintext_length, GUARD_BYTES)){
        fprintf(stderr, "Overrun in decompressed bytes\n");
        frc = 1;
    }

    if(memcmp(ibuf, plaintext_buffer, ilen)){
        fprintf(stderr, "Decompressed plaintext does not match\n");
        frc = 1;
    }

    out:

    free(comparison_buffer);
    free(plaintext_buffer);
    free(compressed_buffer);

    return frc;
}

/*  Test round-trip.  1 on failure, 0 on no failure. */
int test_round(const void* ibuf, unsigned int ilen,
               lzfx_fn compressor, lzfx_fn decompressor){

    u8* compressed_buffer = NULL;
    unsigned int compressed_length;
    u8* plaintext_buffer = NULL;
    unsigned int plaintext_length;

    int rc;
    int frc=0;

    compressed_length = (int)(ilen*1.05) + 16;
    compressed_buffer = (u8*)malloc(compressed_length);

    plaintext_length = ilen;
    plaintext_buffer = (u8*)malloc(plaintext_length);

    rc = compressor(ibuf, ilen, compressed_buffer, &compressed_length);
    if(rc<0){
        fprintf(stderr, "Failed initial compression (code %d)\n", rc);
        frc = 1;
        goto out;
    }

    rc = decompressor(compressed_buffer, compressed_length,
                      plaintext_buffer, &plaintext_length);
    if(rc<0){
        fprintf(stderr, "Failed round-trip decompression (code %d)\n", rc);
        frc = 1;
        goto out;
    }
    
    out:

    free(compressed_buffer);
    free(plaintext_buffer);
    
    return frc;
}

/*  Perform test battery on input (plaintext) buffer.  Prints to stdout.
    
    Return is # of failed tests.
*/
int perform_tests(const void* ibuf, unsigned int ilen, const char* fname){

    int nfailed = 0;
    int rc = 0;

#define DO_TEST(exp, msg) \
  { rc = (exp);     \
    if(rc){         \
        nfailed++;  \
        fprintf(stderr, "\nFail: %s (file %s)\n", (msg), fname); \
    } else { \
        fprintf(stdout, "."); \
    } \
    rc = 0; }

    DO_TEST(test_round(ibuf, ilen, lzf_proxy_comp, lzf_proxy_decomp), "LZF round trip");
    DO_TEST(test_round(ibuf, ilen, lzfx_compress, lzfx_decompress),   "LZFX round trip");

    DO_TEST(test_round(ibuf, ilen, lzfx_compress, lzf_proxy_decomp), "LZFX comp -> LZF decomp");
    DO_TEST(test_round(ibuf, ilen, lzf_proxy_comp, lzfx_decompress), "LZF comp -> LZFX decomp");
 
    DO_TEST(test_bounds(ibuf, ilen, lzfx_compress, lzfx_decompress),   "LZFX overrun check");
    DO_TEST(test_bounds(ibuf, ilen, lzf_proxy_comp, lzf_proxy_decomp), "LZF overrun check");

    fprintf(stdout, "\n");

    return nfailed;
}

/*  Run compression tests on input data chunk

    <imagename> file1 file2 ... filen

    Return code: 0  All passed
                 1  I/O error
                 2  Test failure
*/
int main(int argc, char* argv[]){

    int fd;
    u8 *ibuf = NULL;
    unsigned int ilen = 0;
    
    ssize_t rc;
    ssize_t amt_read = 0;
    int nblocks = 0;

    int argidx;
    int nfailed = 0;

    if(argc<2){
        fprintf(stderr, "Syntax is \"test file1 file2 ... fileN\"\n");
        return 2;
    }

    for(argidx=1; argidx<argc; argidx++){

        amt_read = 0;
        free(ibuf);
        ibuf = NULL;
        ilen = 0;
        nblocks = 0;
        rc = 0;

        if((fd = open(argv[argidx], O_RDONLY)) < 0){
            fprintf(stderr, "Can't open input file \"%s\".\n", argv[argidx]);
            return 2;
        }
        do {
            if((ilen-amt_read) < BLOCKSIZE){
                ibuf = realloc(ibuf, ilen+BLOCKSIZE);
                ilen += BLOCKSIZE;
            }
            rc = read(fd, ibuf, BLOCKSIZE);
            if(rc<0){
                fprintf(stderr, "Read error\n");
                return 2;
            }
            amt_read += rc;
        } while(rc > 0);
        
        ilen = amt_read;

        close(fd);

        nfailed += perform_tests(ibuf, ilen, argv[argidx]);
    }

    if(nfailed){
        fprintf(stdout, "%d test%s failed\n", nfailed, nfailed > 1 ? "s" : "");
    } else {
        fprintf(stdout, "All tests passed\n");
    }

    return !!nfailed;
}







