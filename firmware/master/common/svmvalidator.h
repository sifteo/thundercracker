#ifndef SVMVALIDATOR_H
#define SVMVALIDATOR_H

#include <stdint.h>

class SvmValidator
{
public:
    SvmValidator();
    static unsigned validBytes(void *block, unsigned lenInBytes);
    static bool addressIsValid(uintptr_t address);
    static bool isValid16(uint16_t instr);
    static bool isValid32(uint32_t instr);
};

#endif // SVMVALIDATOR_H
