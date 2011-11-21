
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <speex/speex.h>

#include "audio.gen.h"

#define MAX_FRAME_SIZE      640 // ultra wideband

// #define SAMPLE_RATE 8000
// #define SPEEX_MODE  SPEEX_MODEID_NB

#define SAMPLE_RATE 16000
#define SPEEX_MODE  SPEEX_MODEID_WB

// #define SAMPLE_RATE 32000
// #define SPEEX_MODE  SPEEX_MODEID_UWB

#define SPEEX_OK        0
#define SPEEX_EOS       -1
#define SPEEX_CORRUPT   -2

typedef uint8_t framelength_t;

static char* speex_statestr(int state);
static void version();
static void usage();

int main(int argc, char **argv)
{
  if (argc < 3) {
    usage();
    exit(1);
  }
  
  char *fileinpath = argv[1];
  FILE *fin = fopen(fileinpath, "rb");
  if (fin == 0) {
    fprintf(stderr, "error: couldn't open %s for reading (%s)\n", fileinpath, strerror(errno));
    exit(1);
  }
  
  char *fileoutpath = argv[2];
  FILE *fout = fopen(fileoutpath, "wb");
  if (fout == 0) {
    fprintf(stderr, "error: couldn't open %s for writing (%s)\n", fileoutpath, strerror(errno));
    exit(1);
  }
  
  SpeexBits bits;
  void *decoderState;
  
  speex_bits_init(&bits);
  decoderState = speex_decoder_init(speex_lib_get_mode(SPEEX_MODE));
    
  int framesize;
  speex_decoder_ctl(decoderState, SPEEX_GET_FRAME_SIZE, &framesize); 
  
  int enhancer = 0;
  speex_decoder_ctl(decoderState, SPEEX_SET_ENH, &enhancer); 
  
  short output[MAX_FRAME_SIZE];
  char inbuf[MAX_FRAME_SIZE * 2];
  
  int state = SPEEX_OK;
  int frames = 0;
  framelength_t sz;
  int totalDecodeSize = 0;
  
    for (;;) {
        // file format: framelength_t of size, followed by 'size' bytes of frame data
        fread(&sz, sizeof(uint8_t), 1, fin);
        // fprintf (stderr, "sz: %d\n", sz);
        if (feof(fin))
         break;

        fread(inbuf, 1, sz, fin);
        speex_bits_read_from(&bits, inbuf, sz);
        speex_decode_int(decoderState, &bits, output);
        fwrite(output, sizeof(short), framesize, fout);
        totalDecodeSize += sz;
        frames++;
    }
  
    fclose(fout);

    printf("all done: state %s, frames %d, %d bytes\n", speex_statestr(state), frames, totalDecodeSize);

    speex_bits_destroy(&bits);
    speex_decoder_destroy(decoderState);
    return 0;
}

void version()
{
  const char* speex_version;
  speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void*)&speex_version);
  printf("speexdecoder, based on speex v%s (compiled " __DATE__ ")\n", speex_version);
}

void usage()
{
  version();
  printf("Usage: speexdecoder [options] input_file output_file\n");
}

char* speex_statestr(int state)
{
  switch (state) {
    case SPEEX_OK:      return "OK";
    case SPEEX_EOS:     return "End of stream";
    case SPEEX_CORRUPT: return "Corrupt stream";
    default:            return "Unknown state";
  }
}
