#ifndef SECURERANDOM_H
#define SECURERANDOM_H

#include <stdint.h>


class SecureRandom
{
public:
    static bool generate(uint8_t *buffer, unsigned len);
};


#endif // SECURERANDOM_H
