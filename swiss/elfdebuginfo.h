
#ifndef ELF_DEBUG_INFO_H
#define ELF_DEBUG_INFO_H

#include "elfdefs.h"

#include <vector>
#include <map>
#include <string>

class ELFDebugInfo {
public:
    void clear();
    bool init(const char *elfPath);

    std::string readString(const std::string &section, uint32_t offset) const;
    bool findNearestSymbol(uint32_t address, Elf::Symbol &symbol, std::string &name) const;
    std::string formatAddress(uint32_t address) const;
    bool readROM(uint32_t address, uint8_t *buffer, uint32_t bytes) const;

private:
    typedef std::vector<Elf::SectionHeader> sections_t;
    typedef std::map<std::string, Elf::SectionHeader*> sectionMap_t;

//    Elf::Program program;
    FILE *program;
    sections_t sections;
    sectionMap_t sectionMap;

    static void demangle(std::string &name);
    std::string readString(const Elf::SectionHeader *SI, uint32_t offset) const;
    const Elf::SectionHeader *findSection(const std::string &name) const;

    bool copyProgramBytes(uint32_t byteOffset, uint8_t *dest, uint32_t length) const;
};

#endif // ELF_DEBUG_INFO_H
