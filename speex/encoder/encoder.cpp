
#include "encoder.h"

#include <errno.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <assert.h>

using namespace std;

Encoder::Encoder() :
    headerFile(0),
    sourceFile(0)
{
    encoderState = speex_encoder_init(&speex_nb_mode);

    // int framesize;
    // speex_decoder_ctl(encoderState, SPEEX_GET_FRAME_SIZE, &framesize); 

    int quality = 8; // 8 is the default for speexenc
    speex_encoder_ctl(encoderState, SPEEX_SET_QUALITY, &quality);
    
    speex_bits_init(&bits);
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
        inputs.push_back(line);
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
                    
    string guard(guardName(headerFile));
    headerStream << "#ifndef " << guard << 
                    "\n#define " << guard << "\n\n"
                    "#include <sifteo/audio.h>\n\n";
    /*
     * Get the path to each input in the config file relative
     * to the location of the config file itself.
     */
    string dir(directoryPath(configpath));
    for (vector<string>::iterator i = files.begin(); i != files.end(); i++) {
        string file = *i;
        string path = dir + "/" + file;
        int sz = encodeFile(path, channels, format, headerStream, sourceStream);
        
        sourceStream << "Sifteo::AudioModule " << baseName(file) << " = {\n"
                        "    " << sz << ", // size\n"
                        "    " << baseName(file) << "_data\n"
                        "};\n\n";
    }
    
    headerStream << "\n#endif\n";
    
    headerStream.close();
    sourceStream.close();
    return true;
}

int Encoder::encodeFile(const string &path, int channels, int format, ofstream &headerStream, ofstream &srcStream)    
{
    ifstream instream;
    instream.open(path.c_str(), ios::binary);
    if (instream.fail()) {
        fprintf(stderr, "error: couldn't open %s for reading (%s)\n", path.c_str(), strerror(errno));
        return 0;
    }

    spx_int16_t inbuf[MAX_FRAME_SIZE];
    char outbuf[MAX_FRAME_BYTES];
    char buf[16];
    
    srcStream << "static const uint8_t " << baseName(path) << "_data[] = {";
    headerStream << "extern const Sifteo::AudioModule " << baseName(path) << ";\n";
    
    int bytecount = 0;
    int inlen = instream.seekg(0, ios::end).tellg();
    instream.seekg(0, ios::beg);
    
    while (instream.good()) {
        // note - must use DECODED_FRAME_SIZE here to fetch the appropriate amount of uncompressed data.
        // using the encoder's 'framesize' will not fetch enough data and result in considerable sadness
        int numsamples = readSamples(DECODED_FRAME_SIZE, format, channels, 0 /* lsb */, inbuf, instream);
        if (numsamples == 0) {
            fprintf(stderr, "warning: readSamples() returned 0...\n");
        }
        speex_encode_int(encoderState, inbuf, &bits);

        speex_bits_insert_terminator(&bits); // TBD if we actually need this
        int nbBytes = speex_bits_write(&bits, outbuf, sizeof(outbuf));
        speex_bits_reset(&bits);
        
        assert(nbBytes < 0xFF && "frame is too large for format :(\n");

        // file format: uint8_t of frame size, followed by frame data
        if (!srcStream.is_open()) {
            continue;
        }
        
        sprintf(buf, "0x%02x,", (uint8_t)nbBytes);
        srcStream << "\n    " << buf << "  // frame of " << nbBytes << " bytes";
        bytecount++;

        for (int i = 0; i < nbBytes; i++, bytecount++) {
            if (i % 12 == 0) {
                srcStream << "\n    ";
            }
            sprintf(buf, "0x%02x,", (uint8_t)outbuf[i]);
            srcStream << buf;
        }
    }
    
    srcStream << "\n};\n\n";    
    cout << baseName(path) << ": " << bytecount << " bytes, " << (float)bytecount / (float)inlen << "% of original.\n";
    instream.close();
    return bytecount;
}

// get the containing directory for a file
string Encoder::directoryPath(const char *filepath)
{
    const char *p = strrchr(filepath, '/');
    if (p == NULL) {
        return "";
    }
    
    return string(filepath, p - filepath);
}

// base file name, with no path & no suffix
string Encoder::baseName(const string &filepath)
{
    size_t d = filepath.find_last_of('/');
    if (d == string::npos) { // if no slash, assume the beginning
        d = 0;
    }
    else { // otherwise step past the slash
        d++;
    }
    
    size_t p = filepath.find_last_of('.');
    if (p == string::npos) {
        return "";
    }
    
    return string(filepath, d, p - d);
}

// stolen from stir
string Encoder::guardName(const char *filepath)
{
    char c;
    char prev = '_';
    string guard(1, prev);

    while ((c = *filepath)) {
        c = toupper(c);

        if (isalpha(c)) {
            prev = c;
            guard += prev;
        } else if (prev != '_') {
            prev = '_';
            guard += prev;
        }

        filepath++;
    }
    return guard;
}

int Encoder::readSamples(int framesize, int bits, int channels, int lsb, short *input, std::ifstream & ins)
{
  int rxed, i;
  short *s;
  char in[MAX_FRAME_BYTES * 2]; // account for possible stereo format
  
  rxed = ins.read(in, bits / 8 * channels * framesize).gcount();
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
