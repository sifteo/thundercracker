#ifndef PARTIALANIMATIONDATA_H
#define PARTIALANIMATIONDATA_H

struct EyeData
{
    unsigned lx : 4;
    unsigned ly : 4;
    unsigned rx : 4;
    unsigned ry : 4;
    bool special : 1;
};

const EyeData& getEyeData(char letter);

#endif // PARTIALANIMATIONDATA_H
