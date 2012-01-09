
#include "encoder.h"
#include "filename_utils.h"

#include <errno.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <assert.h>

using namespace std;

// #define SAMPLE_RATE 8000
// #define SPEEX_MODE  SPEEX_MODEID_NB

#define SAMPLE_RATE 16000
#define SPEEX_MODE  SPEEX_MODEID_WB

// #define SAMPLE_RATE 32000
// #define SPEEX_MODE  SPEEX_MODEID_UWB

Encoder::Encoder() :
    headerFile(0),
    sourceFile(0),
    frameSize(0)
{
    encoderState = speex_encoder_init(speex_lib_get_mode(SPEEX_MODE));
    speex_bits_init(&bits);

    int quality = 8; // 8 is the default for speexenc - need to accept this as a param
    speex_encoder_ctl(encoderState, SPEEX_SET_QUALITY, &quality);
    speex_encoder_ctl(encoderState,SPEEX_GET_FRAME_SIZE, &frameSize);
}

Encoder::~Encoder()
{
    speex_bits_destroy(&bits);
    if (encoderState) {
        speex_encoder_destroy(encoderState);
    }
}

bool Encoder::addOutput(const char *path)
{
    string p(path);
    string fileextension = p.substr(p.find_last_of(".") + 1);
    if (fileextension == "h" || fileextension == "hpp") {
        headerFile = path;
        return true;
    }
    else if (fileextension == "cpp" || fileextension == "cc") {
        sourceFile = path;
        return true;
    }
    return false;
}

void Encoder::collectInputs(const char *path, vector<string> &inputs)
{
    ifstream configfile;
    configfile.open(path);
    if (configfile.fail()) {
        fprintf(stderr, "error: couldn't open %s for reading (%s)\n", path, strerror(errno));
        return;
    }
    
    string line;
    while (configfile.good()) {
        getline(configfile, line);
        if (line.length() > 0) {
            inputs.push_back(line);
        }
    }
    configfile.close();
}

bool Encoder::encode(const char *configpath, int channels, int format)
{
    vector<string> files;
    collectInputs(configpath, files);
    if (files.empty()) {
        fprintf(stderr, "no input files specified :(\n");
        return false;
    }
    
    ofstream headerStream;
    if (headerFile) {
        headerStream.open(headerFile, ios::trunc);
        if (headerStream.fail()) {
            fprintf(stderr, "error: couldn't open %s for writing (%s)\n", headerFile, strerror(errno));
            return false;
        }
    }
    
    ofstream sourceStream;
    if (sourceFile) {
        sourceStream.open(sourceFile, ios::trunc);
        if (sourceStream.fail()) {
            fprintf(stderr, "error: couldn't open %s for writing (%s)\n", sourceFile, strerror(errno));
            return false;
        }
    }
    
    /*
     * TODO - move code generation to a separate class :/
     */
    
    sourceStream << "/*"
                    "* Generated automatically - do not edit by hand."
                    "*/\n\n"
                    "#include <sifteo/audio.h>\n\n";
                    
    string guard(FileNameUtils::guardName(headerFile));
    headerStream << "#ifndef " << guard << 
                    "\n#define " << guard << "\n\n"
                    "#include <sifteo/audio.h>\n\n";
    /*
     * Get the path to each input in the config file relative
     * to the location of the config file itself.
     */
    string dir(FileNameUtils::directoryPath(configpath));
    if (!dir.empty()) {
        dir = dir + "/";
    }
    for (vector<string>::iterator i = files.begin(); i != files.end(); i++) {
        string file = *i;
        string path = dir + file;
        int sz = encodeFile(dir, path, channels, format, headerStream, sourceStream);
        
        sourceStream << "_SYSAudioModule " << FileNameUtils::baseName(file) << " = {\n"
                        "    Sample, // type\n"
                        "    " << sz << ", // size\n"
                        "    " << FileNameUtils::baseName(file) << "_data\n"
                        "};\n\n";
    }
    
    headerStream << "\n#endif\n";
    
    headerStream.close();
    sourceStream.close();
    return true;
}

int Encoder::encodeFile(const string & dir, const string &path, int channels, int format, ofstream &headerStream, ofstream &srcStream)
{
    FILE *fin = fopen(path.c_str(), "rb");
    if (fin == 0) {
        fprintf(stderr, "error: couldn't open %s for reading (%s)\n", path.c_str(), strerror(errno));
        return 0;
    }

    string basename = FileNameUtils::baseName(path);

    string encodedOutPath = dir + basename + "_enc.raw";
    FILE *encodedOut = fopen(encodedOutPath.c_str(), "wb");
    if (encodedOut == 0) {
        fprintf(stderr, "error: couldn't open %s for writing (%s)\n", encodedOutPath.c_str(), strerror(errno));
        return 0;
    }

    short inbuf[MAX_FRAME_SIZE];
    char outbuf[MAX_FRAME_BYTES];
    char buf[16];
    
    srcStream << "static const uint8_t " << basename << "_data[] = {";
    headerStream << "extern const _SYSAudioModule " << basename << ";\n";
    
    int bytecount = 0;
    int frameCount = 0;
    
    for (;;) {
        int rx = fread(inbuf, sizeof(short), frameSize, fin);
        if (feof(fin) || rx != frameSize) {
            break;
        }
        
        // encode this frame and write it to our raw encoded file
        speex_bits_reset(&bits);
        speex_encode_int(encoderState, inbuf, &bits);
        
        int nbBytes = speex_bits_write(&bits, outbuf, sizeof(outbuf));
        assert(nbBytes < 0xFF && "frame is too large for format :(\n");
        uint8_t num_bytes = nbBytes & 0xFF;
        fwrite(&num_bytes, sizeof(uint8_t), 1, encodedOut);
        fwrite(outbuf, sizeof(uint8_t), nbBytes, encodedOut);
        bytecount += (nbBytes + 1);
        
        // write out data to a cpp source file for inclusion in a project
        if (srcStream.is_open()) {
            // file format: uint8_t of frame size, followed by frame data
            snprintf(buf, sizeof(buf), "0x%02x,", nbBytes & 0xFF);
            srcStream << "\n    " << buf << "  // frame " << frameCount++ << ", " << nbBytes << " bytes";

            for (int i = 0; i < nbBytes; i++) {
                if (i % 12 == 0) {
                    srcStream << "\n    ";
                }
                snprintf(buf, sizeof(buf), "0x%02x,", (uint8_t)outbuf[i]);
                srcStream << buf;
            }
        }
    }

    fseek(fin, 0, SEEK_END);
    int inlen = ftell(fin);
    srcStream << "\n};\n\n";
    float percent = (float)bytecount / (float)inlen;
    cout << basename << "\n    " << 1. - percent << "% compression (" << bytecount << " bytes)\n";
    
    // cleanup
    fclose(fin);
    fclose(encodedOut);
    return bytecount;
}

#if 0
/*
    Compare the original input with our encoded-then-decoded version of it.
    Determine how much quality has been lost.
*/
void Encoder::calculateSNR(FILE *foriginal, FILE *froundtrip, float *snr, float *seg_snr)
{
    float sigpow = 0, errpow = 0;
    int snr_frames = 0;
    short in_short[MAX_FRAME_SIZE];
    short out_short[MAX_FRAME_SIZE];
    
    *seg_snr = 0;
    
    rewind(foriginal);
    rewind(froundtrip);
    
    while (!feof(foriginal) && !feof(froundtrip)) {
        unsigned origlen      = fread(in_short, sizeof(short), DECODED_FRAME_SIZE, foriginal);
        unsigned roundtriplen = fread(out_short, sizeof(short), DECODED_FRAME_SIZE, froundtrip);
        float s = 0, e = 0;
        
        unsigned comparelen = (origlen < roundtriplen) ? origlen : roundtriplen;
        for (unsigned i = 0; i < comparelen; ++i) {
            s += (float)in_short[i] * in_short[i];
            e += ((float)in_short[i] - out_short[i]) * ((float)in_short[i] - out_short[i]);
        }
        *seg_snr += 10 * log10((s + 1) / (e + 1));
        sigpow += s;
        errpow += e;
        snr_frames++;
    }
    
    *snr = 10 * log10(sigpow / errpow);
    *seg_snr /= snr_frames;
}
#endif
