#include "mappedfile.h"

#ifdef WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <errno.h>
#   include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"

int MappedFile::map(const char *path)
{
#ifdef WIN32

    HANDLE fh = CreateFile(path, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return false;

    unsigned sz = GetFileSize(fh, NULL);

    HANDLE mh = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, sz, NULL);
    if (mh == NULL) {
        CloseHandle(fh);
        return false;
    }

    LPVOID mapping = MapViewOfFile(mh, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sz);
    if (mapping == NULL) {
        CloseHandle(mh);
        CloseHandle(fh);
        return false;
    }

    fileHandle = (uintptr_t) fh;
    mappingHandle = (uintptr_t) mh;
    filesz = sz;
    pData = reinterpret_cast<uint8_t*>(mapping);

#else

    int fh = open(path, O_RDWR | O_CREAT, 0777);
    struct stat st;

    if (fh < 0 || fstat(fh, &st)) {
        if (fh >= 0)
            close(fh);
        return false;
    }

    unsigned sz = (unsigned)st.st_size;

    void *mapping = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fh, 0);
    if (mapping == MAP_FAILED) {
        close(fh);
        return false;
    }

    pData = reinterpret_cast<uint8_t*>(mapping);
    fileHandle = fh;
    filesz = st.st_size;

#endif

    return true;
}

void MappedFile::unmap()
{
    if (!isMapped())
        return;

#ifdef WIN32

    FlushViewOfFile(pData, filesz);
    UnmapViewOfFile(pData);
    CloseHandle((HANDLE) mappingHandle);
    CloseHandle((HANDLE) fileHandle);

#else

    fsync(fileHandle);
    munmap(pData, filesz);
    close(fileHandle);

#endif

    filesz = mappingHandle = fileHandle = 0;
    pData = 0;
}

bool MappedFile::isMapped() const
{
    return filesz > 0;
}

uint8_t* MappedFile::getData(unsigned offset, unsigned &available) const
{
    if (!isMapped() || offset > filesz)
        return 0;

    available = filesz - offset;
    return pData + offset;
}
