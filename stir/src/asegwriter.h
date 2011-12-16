#ifndef _ASEGWRITER_H
#define _ASEGWRITER_H

#include <stdint.h>
#include <fstream>

#include "tile.h"
#include "script.h"
#include "logger.h"

namespace Stir {
    
// Asset Segment Writer - writes the asset segment of a bundle
//   TBD: what should this format actually be, and how does executable
//        code get included.

class ASegWriter {
 public:
    ASegWriter(Logger &log, const char *filename);

    void writeGroup(const Group &group);
    void writeSound(const Sound &sound);

    //void close();

 protected:
    //static const char *indent;
    Logger &mLog;
    std::ofstream mStream;

    //void head();
    //virtual void foot();

    void writeArray(const std::vector<uint8_t> &array);
    
  private:
    int mCurrentID;
    uint32_t mCurrentOffset;
};

}

#endif
