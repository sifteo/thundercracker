#ifndef SVMVALIDATOR_H
#define SVMVALIDATOR_H

#include <stdint.h>

class SvmValidator
{
public:
    SvmValidator();
    unsigned validBytesInBlock(void *block, unsigned lenInBytes);

private:
    bool isValid16(uint16_t instr);
    bool isValid32(uint32_t instr);
};

#endif // SVMVALIDATOR_H
