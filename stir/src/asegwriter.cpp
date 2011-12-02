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
    
    //if (mStream.is_open())
    //    head();
}

void ASegWriter::writeGroup(const Group &group)
{
    fprintf(stdout, "Writing group to asset segment: %s\n", group.getName().c_str());
    
    char buf[32];

    #ifdef __MINGW32__
    sprintf(buf, "0x%016I64x", (long long unsigned int) group.getSignature());
    #else
    sprintf(buf, "0x%016llx", (long long unsigned int) group.getSignature());
    #endif
    
    // TODO: Write header information
    //mStream << group.getLoadstream().size();
    
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
