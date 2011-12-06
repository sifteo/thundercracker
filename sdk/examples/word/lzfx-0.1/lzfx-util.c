#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "lzfx.h"
#include <errno.h>
#include <stdint.h>

#define BLOCKSIZE (1024*1024)

typedef unsigned char u8;

typedef enum {
    MODE_COMPRESS,
    MODE_DECOMPRESS
} fx_mode_t;

typedef enum {
    KIND_FILEHEADER = 0,
    KIND_COMPRESSED = 1,
    KIND_UNCOMPRESSED = 2
} fx_kind_t;

typedef struct {
    int ifd, ofd;
    fx_mode_t mode;
} FX_STATE;

static
void fx_init(FX_STATE *state, int ifd, int ofd, fx_mode_t mode){
    state->ifd = ifd;
    state->ofd = ofd;
    state->mode = mode;
}

/*  Read len bytes from the input file.

    buf:    Output buffer
    len:    # bytes to read

    >=0: bytes read, either 0 (EOF) or len
     <0: Read error
*/
static inline
int fx_read_bytes(const FX_STATE state, void* buf, const size_t len){

    ssize_t rc = 0;
    size_t count = 0;

    do {
        rc = read(state.ifd, ((u8*)buf)+count, len-count);
        count += rc;
    } while(rc>0 && count<len);

    if(rc<0){
        fprintf(stderr, "Read failed: %s\n", strerror(errno));
        return -1;
    }

    if(count>0 && count!=len){
        fprintf(stderr, "Read truncated (%u bytes short)\n", (unsigned int)(len-count));
        return -1;
    }
    return count;
}

/*  Write len bytes from the buffer to the output file.

    buf:    Input buffer
    len:    # of bytes in buf
    
    >=0:    Bytes written
     <0:    Write error
*/
static inline
int fx_write_bytes(const FX_STATE state, const void* buf, const size_t len){

    ssize_t rc = 0;
    size_t count = 0;

    do {
        rc = write(state.ofd, ((u8*)buf)+count, len-count);
        count += rc;
    } while(rc>0 && count<len);

    if(rc<0){
        fprintf(stderr, "Write failed: %s\n", strerror(errno));
        return -1;
    }

    return count;
}

/* Skip len bytes in the input file.

    len:    # of bytes to skip

      0:    Success
     <0:    Read error
*/
static
int fx_skip_bytes(const FX_STATE state, const size_t len){

    off_t rc;

    rc = lseek(state.ifd, len, SEEK_CUR);
    if(rc==((off_t)-1)){
        fprintf(stderr, "Read error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/*  Read a header from the input stream.

    kind_in:    Will contain the header kind
    len_in:     Will contain the block length

    >0:     Read success (10)
     0:     EOF
    <0:     Read error (message printed)
*/
static inline
int fx_read_header(const FX_STATE state, fx_kind_t *kind_in, uint32_t *len_in){

    int rc;
    uint16_t kind;
    uint32_t len;
    u8 head[10];

    rc = fx_read_bytes(state, head, 10);
    if(rc<=0) return rc; /* 0 = EOF; <0 = already printed error */

    if(head[0]!='L' || head[1]!='Z' || head[2]!='F' || head[3]!='X'){
        fprintf(stderr, "Illegal header %X %X %X %X\n",
                (int)head[0], (int)head[1], (int)head[2], (int)head[3]);
        return -1;
    }

    kind = (head[4]<<8) | head[5];
    len = (head[6]<<24) | (head[7]<<16) | (head[8]<<8) | head[9];

    *kind_in = kind;
    *len_in = len;

    return rc;
}

/*  Write a block to output file, adding the header.

    kind_in:    Header kind
    len:        Block length
    data:       Block data

     0:     Write success
    <0:     Write error (message printed)
*/
static inline
int fx_write_block(const FX_STATE state, const fx_kind_t kind_in, 
                   const uint32_t len, const void* data){

    const uint16_t kind = kind_in;
    u8 head[] = {'L','Z','F','X',   0, 0,   0, 0, 0, 0};

    head[4] = kind >> 8; head[5] = kind;
    head[6] = len >> 24; head[7] =  len >> 16;
    head[8] = len >> 8; head[9] = len;

    if(fx_write_bytes(state, head, 10) < 0) return -1;
    if(fx_write_bytes(state, data, len) < 0) return -1;

    return 0;
}

/*  Decompress a block (KIND_COMPRESSED) to the output file.

    ibuf:   The block (including 4-byte leader)
    len:    Total block length

     0:  Success
    <0:  Error (message printed)
*/
static
int fx_decompress_block(const FX_STATE state, const u8 *ibuf, const size_t len){

    static u8* obuf;
    static size_t obuf_len;

    uint32_t usize;
    unsigned int usize_real;
    int rc;

    if(len<4){
        fprintf(stderr, "Compressed size truncated\n");
        return -2;
    }

    usize = (ibuf[0]<<24) | (ibuf[1]<<16) | (ibuf[2]<<8) | ibuf[3];

    if(usize>obuf_len){
        obuf = (u8*)realloc(obuf, usize);
        if(obuf==NULL) return -1;  /* This leaks but we quit right away */
        obuf_len = usize;
    }
    
    usize_real = usize;

    rc = lzfx_decompress(ibuf+4, len-4, obuf, &usize_real);
    if(rc<0){
        fprintf(stderr, "Decompression failed: code %d\n", rc);
        return -2;
    }
    if(usize_real != usize){
        fprintf(stderr, "Decompressed data has wrong length (%d vs expected %d)\n", (int)usize_real, (int)usize);
        return -2;
    }

    rc = fx_write_bytes(state, obuf, usize);
    if(rc<0) return -1;

    return 0;
}

static inline
int mem_resize(u8** buf, size_t *ilen, const size_t olen){
    void* tbuf;
    if(olen>*ilen){
        tbuf = realloc(*buf, olen);
        if(tbuf==NULL){
            fprintf(stderr, "Can't allocate memory (%lu bytes)\n", (unsigned long)olen);
            return -1;
        }
        *buf = tbuf;
        *ilen = olen;
    }
    return 0;
}

/* Compress a block of raw data and store it in the output file.

    ibuf:   Raw data block to compress
    ilen:   Length of data block

    0:  Success
    <0: Error (message printed)
*/
static
int fx_compress_block(const FX_STATE state, const u8* ibuf,
                      const uint32_t ilen){

    static u8* obuf;
    static size_t obuf_len;

    unsigned int compressed_len;
    int rc;

    if(ilen<=4){
        rc = fx_write_block(state, KIND_UNCOMPRESSED, ilen, ibuf);
        return rc<0 ? -1 : 0;
    }

    rc = mem_resize(&obuf, &obuf_len, ilen);
    if(rc<0) return -1;

    /* 4-byte space to store the usize */
    compressed_len = ilen - 4; 

    rc = lzfx_compress(ibuf, ilen, obuf+4, &compressed_len);
    if(rc<0 && rc != LZFX_ESIZE){
        fprintf(stderr, "Compression error (code %d)\n", rc);
        return -1;
    }

    if(rc == LZFX_ESIZE || !compressed_len){

        rc = fx_write_block(state, KIND_UNCOMPRESSED, ilen, ibuf);
        if(rc<0) return -1;
    
    } else {
        
        obuf[0] = ilen >> 24;
        obuf[1] = ilen >> 16;
        obuf[2] = ilen >> 8;
        obuf[3] = ilen;

        rc = fx_write_block(state, KIND_COMPRESSED, compressed_len+4, obuf);
        if(rc<0) return -1;
    }

    return 0;
}


/*  Compress from input to output file.

    0:  Success
    <0: Failure (message printed)
*/
int fx_create(const FX_STATE state){

    unsigned long blockno = 0;
    ssize_t rc = 0;
    size_t count = 0;
    u8* ibuf = (u8*)malloc(BLOCKSIZE);

    do {
        rc = 0;
        count = 0;
        do {
            rc = read(state.ifd, ibuf, BLOCKSIZE-count);
            if(rc<0){
                fprintf(stderr, "Read error: %s\n", strerror(rc));
                goto failed;
            }
            count += rc;
        } while(rc > 0 && count < BLOCKSIZE);

        blockno++;

        if(count>0){
            rc = fx_compress_block(state, ibuf, count);
            if(rc<0) goto failed;
        }

    } while(count==BLOCKSIZE);

    free(ibuf);
    return 0;

failed:
    fprintf(stderr, "Compression failed at byte %lu\n", blockno*BLOCKSIZE + count);
    free(ibuf);
    return -1;
}

/*  Read an LZFX file

    0: success
    <0: Failure (message printed)
*/
int fx_read(const FX_STATE state){

    int rc;
    fx_kind_t kind = 0;
    uint32_t blocksize = 0;

    static u8* ibuf;
    static size_t ilen;

    while(1){
        rc = fx_read_header(state, &kind, &blocksize);
        if(rc==0) break; /* EOF */
        if(rc<0) return -1;
        if(blocksize==0) continue;

        switch(kind){
        
        case KIND_UNCOMPRESSED:
            rc = mem_resize(&ibuf, &ilen, blocksize);
            if(rc<0) return -1;

            rc = fx_read_bytes(state, ibuf, blocksize);
            if(rc<0) return -1;
            if(rc==0){
                fprintf(stderr, "EOF after block header (tried to read %d bytes)\n", blocksize);
                return -1;
            }

            rc = fx_write_bytes(state, ibuf, blocksize);
            if(rc<0) return -1;
            break;

        case KIND_COMPRESSED:
            rc = mem_resize(&ibuf, &ilen, blocksize);
            if(rc<0) return -1;

            rc = fx_read_bytes(state, ibuf, blocksize);
            if(rc<0) return -1;
            if(rc==0){
                fprintf(stderr, "EOF after block header\n");
                return -1;
            }
            rc = fx_decompress_block(state, ibuf, blocksize);
            if(rc<0) return -1;
            break;

        default:
            rc = fx_skip_bytes(state, blocksize);
            if(rc<0) return -1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]){

    int rc;
    int ifd, ofd;
    fx_mode_t mode;
    FX_STATE state;

    fprintf(stderr, "LZFX compression utility 0.1\n"
                    "http://lzfx.googlecode.com\n"
                    "*********************************\n"
                    "  THIS IS A DEVELOPMENT RELEASE\n"
                    "  DO NOT USE ON CRITICAL DATA\n"
                    "*********************************\n");
    
    if(argc!=4){
        fprintf(stderr, "Syntax is lzfx <namein> <nameout> c|d\n");
        return 1;
    }

    ifd = open(argv[1], O_RDONLY);
    if(ifd<0){
        fprintf(stderr, "Can't open input file\n");
        return 1;
    }

    ofd = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(ofd<0){
        fprintf(stderr, "Can't open output file for write\n");
        return 1;
    }

    if(!strcmp(argv[3], "c")){
        mode = MODE_COMPRESS;
    } else if(!strcmp(argv[3], "d")){
        mode = MODE_DECOMPRESS;
    } else {
        fprintf(stderr, "Illegal mode (must be 'c' or 'd')\n");
        return 1;
    }

    fx_init(&state, ifd, ofd, mode);

    switch(mode){
        case MODE_COMPRESS:
            rc = fx_create(state);
            break;
        case MODE_DECOMPRESS:
            rc = fx_read(state);
            break;
    }

    return rc ? 1 : 0;
}

