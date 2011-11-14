
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <speex/speex.h>

#include "encoder.h"

// #define DEBUGGING

static void version();
static void usage();
#ifdef DEBUGGING
static void printEncoderConfig(void *encoder);
#endif

int main(int argc, char **argv)
{
	int channels = 1;               // mono
	int format = 16;                // 16 bit format
    const char *infilepath = 0;
    
    Encoder enc;
    
	for (int c = 1; c < argc; c++) {
        char *arg = argv[c];
        
        if (!strcmp(arg, "-h")) {
            usage();
            return 0;
        }
        
        if (!strcmp(arg, "-v")) {
            version();
            return 0;
        }
        
        if (!strcmp(arg, "-c") && argv[c + 1]) {
            channels = atoi(argv[c + 1]);
            continue;
        }
        
        if (!strcmp(arg, "-b") && argv[c + 1]) {
            format = atoi(argv[c + 1]);
            continue;
        }
        
        if (!strcmp(arg, "-o") && argv[c + 1]) {
            if (enc.addOutput(argv[c + 1])) {
                c++;
                continue;
            }
            else {
                fprintf(stderr, "Unrecognized or duplicate output file: '%s'", argv[c+1]);
                return 1;
            }
        }
        
        if (arg[0] == '-') {
            fprintf(stderr, "Unrecognized option: '%s'", arg);
            return 1;
        }
        
        infilepath = arg;
    }
    
    if (!infilepath) {
        fprintf(stderr, "error: must st least specify an input file :(\n");
        usage();
        exit(1);
    }
    
    return enc.encode(infilepath, channels, format) ? 0 : 1;
}

void version()
{
  const char* speex_version;
  speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void*)&speex_version);
  fprintf(stderr, "speexencoder, based on speex v%s (compiled " __DATE__ ")\n", speex_version);
}

void usage()
{
	fprintf(stderr, "usage: speexencoder [OPTIONS] input_file\n"
                    "Options:\n"
                    "  -h              print this help and exit\n"
                    "  -v              print version and exit\n\n"
                    "  -c              specify the number of channels in the input, defaults to 1\n"
                    "  -m              specify the format of the input, defaults to signed 16 bit\n"
                    "  -o FILE.cpp     generate a C++ source file with your audio data\n"
                    "  -o FILE.h       generate a C++ header file that describes your audio data\n\n");
}

#ifdef DEBUGGING
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
#endif
