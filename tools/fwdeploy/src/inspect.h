#ifndef INSPECT_H
#define INSPECT_H

#include <iostream>

class Inspect
{
public:
    Inspect();

    void dumpSections(const char *path);

private:
    void dumpContainer(std::istream &is);
    bool dumpFirmware(std::istream &is, unsigned sectionLen);
};

#endif // INSPECT_H
