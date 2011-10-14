
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <speex/speex.h>

// max sizes from speexenc
#define MAX_FRAME_SIZE      2000
#define MAX_FRAME_BYTES     2000
#define DECODED_FRAME_SIZE  160

#define SPEEX_OK            0
#define SPEEX_EOS           -1
#define SPEEX_CORRUPT       -2

static int read_samples(int framesize, int bits, int channels, int lsb, short *input, FILE *fin);
static void version();
static void usage();
static void printEncoderConfig(void *encoder);

int main(int argc, char **argv)
{
  if (argc < 3) {
    usage();
    exit(1);
  }
  
  char *infilepath = argv[1];
  FILE *fin = fopen(infilepath, "r");
  if (fin == 0) {
    fprintf(stderr, "error: couldn't open %s for reading (%s)\n", infilepath, strerror(errno));
    exit(1);
  }
  
  char *outfilepath = argv[2];
  FILE *fout = fopen(outfilepath, "w");
  if (fout == 0) {
    fprintf(stderr, "error: couldn't open %s for writing (%s)\n", outfilepath, strerror(errno));
    exit(1);
  }
  
  SpeexBits bits;
  void *encoderState = speex_encoder_init(&speex_nb_mode);
  
  int framesize;
  speex_decoder_ctl(encoderState, SPEEX_GET_FRAME_SIZE, &framesize); 
  
  int quality = 8; // 8 is the default for speexenc
  speex_encoder_ctl(encoderState, SPEEX_SET_QUALITY, &quality);
  
  // printEncoderConfig(encoderState);
  
  spx_int16_t inbuf[MAX_FRAME_SIZE];
  char outbuf[MAX_FRAME_BYTES];
  
  speex_bits_init(&bits);
  
  // TODO - get these from input
  int channels = 1;   // mono
  int format = 16;    // 16 bit format
  
  // TODO - write out any appropriate header info if necessary
  
  while (!feof(fin) && !ferror(fin)) {
    // note - must use DECODED_FRAME_SIZE here to fetch the appropriate amount of uncompressed data.
    // using the encoder's 'framesize' will not fetch enough data and result in considerable sadness
    int numsamples = read_samples(DECODED_FRAME_SIZE, format, channels, 0 /* lsb */, inbuf, fin);
    speex_encode_int(encoderState, inbuf, &bits);
    
    speex_bits_insert_terminator(&bits); // terminate the previous frame - TBD if we actually need this
    int nbBytes = speex_bits_write(&bits, outbuf, sizeof(outbuf));
    speex_bits_reset(&bits);
    // file format: int of frame size, followed by frame data
    fwrite(&nbBytes, sizeof(int), 1, fout);
    fwrite(outbuf, 1, nbBytes, fout);
  }
  
  
  struct stat instatbuf;
  struct stat outstatbuf;
  
  fstat(fileno(fin), &instatbuf);
  fstat(fileno(fout), &outstatbuf);
  
  float percent = (float)outstatbuf.st_size / (float)instatbuf.st_size;
  
  printf("all done, compressed size: %d bytes (%f%% of original)\n", (int)outstatbuf.st_size, percent);
  
  fclose(fin);
  fclose(fout);
  
  speex_bits_destroy(&bits);
  speex_encoder_destroy(encoderState);
  
  return 0;
}

int read_samples(int framesize, int bits, int channels, int lsb, short *input, FILE *fin)
{
  int rxed, i;
  short *s;
  unsigned char in[MAX_FRAME_BYTES * 2]; // account for possible stereo format
  
  rxed = fread(in, 1, bits / 8 * channels * framesize, fin);
  rxed /= bits / 8 * channels;
  if (rxed == 0) {
    return 0;
  }
  
  s = (short*)in;
  
  // TODO - endian conversion here if necessary
  
  // TODO - memcpy & memset these
  for (i = 0; i < framesize * channels; i++) {
    input[i] = s[i];
  }

  for (i = rxed * channels; i < framesize * channels; i++) {
    input[i] = 0;
  }
  return rxed;
}

void version()
{
  const char* speex_version;
  speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void*)&speex_version);
  printf("speexencoder, based on speex v%s (compiled " __DATE__ ")\n", speex_version);
}

void usage()
{
  version();
  printf("Usage: speexencoder [options] input_file output_file\n");
}

void printEncoderConfig(void *encoder)
{
  int val;
  speex_decoder_ctl(encoder, SPEEX_GET_FRAME_SIZE, &val); 
  printf("framesize: %d\n", val);
  
  speex_encoder_ctl(encoder, SPEEX_GET_COMPLEXITY, &val);
  printf("complexity: %d\n", val);
  
  speex_encoder_ctl(encoder, SPEEX_GET_SAMPLING_RATE, &val);
  printf("sampling rate: %d\n", val);
  
  speex_encoder_ctl(encoder, SPEEX_GET_VBR, &val);
  printf("vbr: %d\n", val);
  
  speex_encoder_ctl(encoder, SPEEX_GET_VAD, &val);
  printf("vad: %d\n", val);
  
  speex_encoder_ctl(encoder, SPEEX_GET_DTX, &val);
  printf("dtx: %d\n", val);
}
