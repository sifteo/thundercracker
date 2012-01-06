#include "asegwriter.h"
#include "speexencoder.h"

using namespace Stir;


ASegWriter::ASegWriter(Logger &log, const char *filename)
    : mLog(log),
      mCurrentID(0),
      mCurrentOffset(512)
{
    if (filename) {
        mStream.open(filename, std::ios::binary);
        if (!mStream.is_open())
            log.error("Error opening output file '%s'", filename);
    }
    
    if (mStream.is_open()) {
        // reserve space for the header.
        // TODO: what if we run out of room???
        mStream.seekp(512);
        //head();
    }
}

void ASegWriter::writeGroup(const Group &group)
{
    fprintf(stdout, "Writing group to asset segment: %s\n", group.getName().c_str());
    fprintf(stdout, "  name: %s\n", group.getName().c_str());
    fprintf(stdout, "  size: %lu\n", (long unsigned)group.getLoadstream().size());
    
    char buf[32];

    #ifdef __MINGW32__
    sprintf(buf, "0x%016I64x", (long long unsigned int) group.getSignature());
    #else
    sprintf(buf, "0x%016llx", (long long unsigned int) group.getSignature());
    #endif
    
    // TODO: Write header information
    //mStream << group.getLoadstream().size();
    
    
    uint32_t val = 0;
    uint16_t val16 = 0;
    uint64_t val64 = 0;
    
    mStream.seekp(mCurrentID * 8);
    
    val = 1; // type (1 = asset group)
    mStream.write((char *)&val, sizeof(uint32_t));
    val = mCurrentOffset; // offset
    mStream.write((char *)&val, sizeof(uint32_t));
    
    // TODO: serialize integers in network format.
    mStream.seekp(mCurrentOffset);
    // write header (see AssetGroupHeader in firmware)
    val16 = group.getPool().size();
    mStream.write((char *)&val16, sizeof(uint16_t));
    val16 = 0;
    mStream.write((char *)&val16, sizeof(uint16_t));
    val = group.getLoadstream().size();
    mStream.write((char *)&val, sizeof(uint32_t));
    val64 = group.getSignature();
    mStream.write((char *)&val64, sizeof(uint64_t));
    // write data
    writeArray(group.getLoadstream());
    
    mCurrentID++;
    
    mCurrentOffset += group.getLoadstream().size() + 16;
    uint32_t pad = mCurrentOffset % 512;
    mCurrentOffset += pad;
}

void ASegWriter::writeSound(const Sound &sound)
{
    fprintf(stdout, "WRITING SOUND TO ASSET: %s\n", sound.getName().c_str());
    
    int channels = 1;               // mono
    int format = 16;                // 16 bit format
    
    
    uint32_t val = 0;
    
    mStream.seekp(mCurrentID * 8);
    
    // write offset to index
    val = 2; // type (2 = audio)
    mStream.write((char *)&val, sizeof(uint32_t));
    val = mCurrentOffset - sizeof(uint32_t); // offset
    mStream.write((char *)&val, sizeof(uint32_t));
    
    // write audio data
    //mStream.seekp(20480 + sizeof(uint32_t));
    mStream.seekp(mCurrentOffset);
    SpeexEncoder encoder(sound.getQuality());
    uint32_t size = encoder.encodeFile(sound.getFile(), channels, format, mStream);
    
    //mStream.seekp(20480);
    mStream.seekp(mCurrentOffset - sizeof(uint32_t));
    mStream.write((char *)&size, sizeof(uint32_t));
    
    mCurrentID++;
    
    mCurrentOffset += size;
    uint32_t pad = mCurrentOffset % 512;
    mCurrentOffset += pad;
}

void ASegWriter::writeArray(const std::vector<uint8_t> &array)
{
    //char buf[8];

    //mStream << indent;

    for (unsigned i = 0; i < array.size(); i++) {
        //if (i && !(i % 16))
        //    mStream << "\n" << indent;

        //sprintf(buf, "0x%02x,", array[i]);
        //mStream << buf;
        mStream << array[i];
    }

    //mStream << "\n";
}



/* TODO: Speex encoder should be factored out into a separate class. */

