
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <speex/speex.h>

#define MAX_FRAME_SIZE      2000 // from speexdec

#define SPEEX_OK        0
#define SPEEX_EOS       -1
#define SPEEX_CORRUPT   -2

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
  FILE *fin = fopen(fileinpath, "r");
  if (fin == 0) {
    fprintf(stderr, "error: couldn't open %s for reading (%s)\n", fileinpath, strerror(errno));
    exit(1);
  }
  
  char *fileoutpath = argv[2];
  FILE *fout = fopen(fileoutpath, "w");
  if (fout == 0) {
    fprintf(stderr, "error: couldn't open %s for writing (%s)\n", fileoutpath, strerror(errno));
    exit(1);
  }
  
  SpeexBits bits;
  void *decoderState;
  
  speex_bits_init(&bits);
  decoderState = speex_decoder_init(&speex_nb_mode);
    
  int framesize;
  speex_decoder_ctl(decoderState, SPEEX_GET_FRAME_SIZE, &framesize); 
  
  int enhancer = 1;
  speex_decoder_ctl(decoderState, SPEEX_SET_ENH, &enhancer); 
  
  short output[MAX_FRAME_SIZE];
  char inbuf[MAX_FRAME_SIZE * 2];
  
  int state = SPEEX_OK;
  int frames = 0;
  int channels = 1; // mono for now
  int i;
  int sz;
  int totalDecodeSize = 0;
  
  // file format: int of size, followed by 'size' bytes of frame data
  fread(&sz, 1, sizeof(int), fin);
  
  while (sz > 0 && !feof(fin) && !ferror(fin) && state == SPEEX_OK) {    
    int res = fread(inbuf, 1, sz, fin);
    if (res != sz) {
      fprintf(stderr, "read error... %d, %s\n", res, strerror(errno));
      break;
    }
    
    speex_bits_read_from(&bits, inbuf, sz);
    state = speex_decode_int(decoderState, &bits, output);
    
    if (speex_bits_remaining(&bits) < 0) {
      fprintf (stderr, "Decoding overflow: corrupted stream?\n");
      break;
    }
    
    fwrite(output, sizeof(short), framesize * channels, fout);
    totalDecodeSize += framesize * channels * sizeof(short);
    frames++;
    fread(&sz, 1, sizeof(int), fin); // next frame size
  }
  
  fclose(fout);
  
  printf("all done: state %s, frames %d, totalDecodeSize %d\n", speex_statestr(state), frames, totalDecodeSize);
  
  speex_bits_destroy(&bits);
  speex_decoder_destroy(decoderState);
  
  return -1;
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
