#ifndef _AES128_H
#define _AES128_H

#include <stdint.h>

class AES128
{
public:
    static const unsigned BLOCK_SIZE = 16;

    static void expandKey(uint32_t* expkey, const uint32_t* key);

    static void encryptBlock(uint8_t *dest, const uint8_t *src, const uint32_t* expkey);
    static void decryptBlock(uint32_t* dest, const uint32_t* src, const uint32_t* expkey);

    static void xorBlock(uint8_t* dest, const uint8_t* src) {

        uint32_t* dest32 = reinterpret_cast<uint32_t*>(dest);
        const uint32_t* src32 = reinterpret_cast<const uint32_t*>(src);

        dest32[0] ^= src32[0];
        dest32[1] ^= src32[1];
        dest32[2] ^= src32[2];
        dest32[3] ^= src32[3];
    }
};

#endif // _AES128_H
