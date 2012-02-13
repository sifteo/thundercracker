#ifndef ELFREADER_H
#define ELFREADER_H

#include "elfdefs.h"

class ElfReader
{
public:
    ElfReader();

    void load(unsigned addr, unsigned len);
    void dumpHeader(const Elf::FileHeader & header);
    void dumpProgramHeader(const Elf::ProgramHeader & header);
    void dumpSectionHeader(const Elf::SectionHeader & header);
};

#endif // ELFREADER_H
