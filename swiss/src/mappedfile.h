#ifndef MAPPEDFILE_H
#define MAPPEDFILE_H

#include <stdint.h>

class MappedFile
{
public:
    MappedFile() :
        fileHandle(0),
        mappingHandle(0),
        pData(0),
        filesz(0)
    {}

    int map(const char *path);
    void unmap();
    bool isMapped() const;

    uint8_t* getData(unsigned offset, unsigned &available) const;

private:
    uintptr_t fileHandle;
    uintptr_t mappingHandle;
    uint8_t *pData;
    unsigned filesz;
};

#endif // MAPPEDFILE_H
