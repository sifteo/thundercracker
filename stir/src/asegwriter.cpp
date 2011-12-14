#include "asegwriter.h"

using namespace Stir;


ASegWriter::ASegWriter(Logger &log, const char *filename)
    : mLog(log)
{
    if (filename) {
        mStream.open(filename);
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
    fprintf(stdout, "  size: %lu\n", group.getLoadstream().size());
    
    char buf[32];

    #ifdef __MINGW32__
    sprintf(buf, "0x%016I64x", (long long unsigned int) group.getSignature());
    #else
    sprintf(buf, "0x%016llx", (long long unsigned int) group.getSignature());
    #endif
    
    // TODO: Write header information
    //mStream << group.getLoadstream().size();
    
    uint32_t type = 1;
    uint32_t val;
    
    mStream.seekp(0);
    mStream.write((char *)&type, sizeof(uint32_t));
    val = sizeof(uint32_t) * 2; // len of asset group = 8 (offset and size)
    mStream.write((char *)&val, sizeof(uint32_t));
    val = 512; // offset
    mStream.write((char *)&val, sizeof(uint32_t));
    val = group.getLoadstream().size(); // size
    mStream.write((char *)&val, sizeof(uint32_t));
    
    mStream.seekp(512);
    writeArray(group.getLoadstream());
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
