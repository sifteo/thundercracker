#ifndef PARTIALANIMATIONDATA_H
#define PARTIALANIMATIONDATA_H

struct EyeData
{
    unsigned lx : 7;
    unsigned ly : 7;
    unsigned rx : 7;
    unsigned ry : 7;
    bool special : 1;
};

const EyeData& getEyeData(char letter);

#endif // PARTIALANIMATIONDATA_H
