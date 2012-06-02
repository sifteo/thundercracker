
#include "aes128.h"

// Matrix Sbox for the Sbox operation in encryption procedure
static const uint8_t Sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Matrix INVSBOX for the procedure of decryption
static const uint8_t InvSbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
    0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
    0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
    0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
    0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
    0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
    0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
    0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
    0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
    0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
    0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
    0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Matrix of number necessary for the keyschedule procedure
static const uint32_t rcon[10] = {
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000,
    0x1b000000, 0x36000000
};

/* Private macro -------------------------------------------------------------*/

static inline uint8_t byte3(uint32_t x) {
    return x & 0xff;
}

static inline uint8_t byte2(uint32_t x) {
    return (x >> 8) & 0xff;
}

static inline uint8_t byte1(uint32_t x) {
    return (x >> 16) & 0xff;
}

static inline uint8_t byte0(uint32_t x) {
    return (x >> 24) & 0xff;
}

static inline uint32_t bytesToWord(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

// Multiply for 2 each byte of a WORD32 working in parallel mode on each one
static inline uint32_t Xtime(uint32_t x) {
    return (((x) & 0x7f7f7f7f) << 1) ^ ((((x) & 0x80808080) >> 7) * 0x0000001b);
}

// Right shift x of n bytes
static inline uint32_t upr(uint32_t x, unsigned n) {
    return (x >> 8 * n) | (x << (32 - 8 * n));
}

// Develop of the matrix necessary for the MixColomn procedure
static inline uint32_t fwd_mcol(uint32_t x) {
    return Xtime(x) ^ (upr((x ^ Xtime(x)), 3)) ^ (upr(x, 2)) ^ (upr(x, 1));
}

// Develop of the matrix necessary for the InvMixColomn procedure
// TODO: clean this
#define inv_mcol(x)  (f2=Xtime(x),f4=Xtime(f2),f8=Xtime(f4),(x)^=f8, f2^=f4^f8^(upr((f2^(x)),3))^(upr((f4^(x)),2))^(upr((x),1)))

static inline uint32_t rot3(uint32_t x) {
    return (x << 8 ) | (x >> 24);
}

static inline uint32_t rot2(uint32_t x) {
    return (x << 16) | (x >> 16);
}

static inline uint32_t rot1(uint32_t x) {
    return (x << 24 ) | (x >> 8);
}

/*
 * According to key, computes the expanded key (expkey).
 */
void AES128::expandKey(uint32_t* expkey, const uint32_t* key)
{
    register uint32_t* local_pointer = expkey;
    register int i = 0;
    register uint32_t copy0;
    register uint32_t copy1;
    register uint32_t copy2;
    register uint32_t copy3;

    copy0 = key[0];
    copy1 = key[1];
    copy2 = key[2];
    copy3 = key[3];

    local_pointer[0] = copy0;
    local_pointer[1] = copy1;
    local_pointer[2] = copy2;
    local_pointer[3] = copy3;

    for (;i < 10;) {
        copy0 ^= bytesToWord( Sbox[byte1(copy3)],
                                  Sbox[byte2(copy3)],
                                  Sbox[byte3(copy3)],
                                  Sbox[byte0(copy3)]) ^ rcon[i++];
        copy1 ^= copy0;
        copy2 ^= copy1;
        copy3 ^= copy2;
        local_pointer += 4;
        local_pointer[0] = copy0;
        local_pointer[1] = copy1;
        local_pointer[2] = copy2;
        local_pointer[3] = copy3;
    }
}

void AES128::encryptBlock(uint8_t* dest, const uint8_t* src, const uint32_t* expkey)
{
    uint32_t* dest32 = reinterpret_cast<uint32_t*>(dest);
    const uint32_t* src32 = reinterpret_cast<const uint32_t*>(src);

    register uint32_t t0;
    register uint32_t t1;
    register uint32_t t2;
    register uint32_t t3;
    register uint32_t s0;
    register uint32_t s1;
    register uint32_t s2;
    register uint32_t s3;
    register int r = 10; // Round counter
    register const uint32_t* local_pointer = expkey;

    s0 = src32[0] ^ local_pointer[0];
    s1 = src32[1] ^ local_pointer[1];
    s2 = src32[2] ^ local_pointer[2];
    s3 = src32[3] ^ local_pointer[3];
    local_pointer += 4;

    // ADD KEY before start round
    for (;;) {
        t0 = bytesToWord( Sbox[byte0(s0)],
                              Sbox[byte1(s1)],
                              Sbox[byte2(s2)],
                              Sbox[byte3(s3)]);
        t1 = bytesToWord( Sbox[byte0(s1)],
                              Sbox[byte1(s2)],
                              Sbox[byte2(s3)],
                              Sbox[byte3(s0)]);
        t2 = bytesToWord( Sbox[byte0(s2)],
                              Sbox[byte1(s3)],
                              Sbox[byte2(s0)],
                              Sbox[byte3(s1)]);
        t3 = bytesToWord( Sbox[byte0(s3)],
                              Sbox[byte1(s0)],
                              Sbox[byte2(s1)],
                              Sbox[byte3(s2)]);
        // End of SBOX + Shift ROW

        s0 = fwd_mcol(t0) ^ local_pointer[0];
        s1 = fwd_mcol(t1) ^ local_pointer[1];
        s2 = fwd_mcol(t2) ^ local_pointer[2];
        s3 = fwd_mcol(t3) ^ local_pointer[3];
        // End of mix column

        local_pointer += 4;
        if ( --r == 1) {
            break;
        }

    }

    // Start Last round
    t0 = bytesToWord( Sbox[byte0(s0)],
                          Sbox[byte1(s1)],
                          Sbox[byte2(s2)],
                          Sbox[byte3(s3)]);
    t1 = bytesToWord( Sbox[byte0(s1)],
                          Sbox[byte1(s2)],
                          Sbox[byte2(s3)],
                          Sbox[byte3(s0)]);
    t2 = bytesToWord( Sbox[byte0(s2)],
                          Sbox[byte1(s3)],
                          Sbox[byte2(s0)],
                          Sbox[byte3(s1)]);
    t3 = bytesToWord( Sbox[byte0(s3)],
                          Sbox[byte1(s0)],
                          Sbox[byte2(s1)],
                          Sbox[byte3(s2)]);

    t0 ^= local_pointer[0];
    t1 ^= local_pointer[1];
    t2 ^= local_pointer[2];
    t3 ^= local_pointer[3];

    // Store of the result of encryption
    dest32[0] = t0;
    dest32[1] = t1;
    dest32[2] = t2;
    dest32[3] = t3;
}

/*
* Decrypt one block of 16 bytes from 'src' to 'dest' using the expanded key 'expkey'
*/
void AES128::decryptBlock(uint32_t* dest, const uint32_t *src, const uint32_t *expkey)
{
    /*
     * TODO: have not looked at what kind of assembly this is generating...
     */
    register uint32_t t0;
    register uint32_t t1;
    register uint32_t t2;
    register uint32_t t3;
    register uint32_t s0;
    register uint32_t s1;
    register uint32_t s2;
    register uint32_t s3;
    register int r =  10; // Count the round
    register const uint32_t* local_pointer = expkey + 40;
    uint32_t f2, f4, f8;

    s0 = src[0] ^ local_pointer[0];
    s1 = src[1] ^ local_pointer[1];
    s2 = src[2] ^ local_pointer[2];
    s3 = src[3] ^ local_pointer[3];

    // First add key: before start the rounds
    local_pointer -= 8;

    // Start round
    for (;;) {
        t0 = bytesToWord( InvSbox[byte0(s0)],
                              InvSbox[byte1(s3)],
                              InvSbox[byte2(s2)],
                              InvSbox[byte3(s1)]) ^ local_pointer[4];
        t1 = bytesToWord( InvSbox[byte0(s1)],
                              InvSbox[byte1(s0)],
                              InvSbox[byte2(s3)],
                              InvSbox[byte3(s2)]) ^ local_pointer[5];
        t2 = bytesToWord( InvSbox[byte0(s2)],
                              InvSbox[byte1(s1)],
                              InvSbox[byte2(s0)],
                              InvSbox[byte3(s3)]) ^ local_pointer[6];
        t3 = bytesToWord( InvSbox[byte0(s3)],
                              InvSbox[byte1(s2)],
                              InvSbox[byte2(s1)],
                              InvSbox[byte3(s0)]) ^ local_pointer[7];

        // End of InvSbox,  INVshiftRow,  add key
        s0 = inv_mcol(t0);
        s1 = inv_mcol(t1);
        s2 = inv_mcol(t2);
        s3 = inv_mcol(t3);

        // End of INVMix column
        local_pointer -= 4; // Follow the key sheduler to choose the right round key

        if (--r == 1) {
            break;
        }

    } // End of round

    // Start last round :is the only one different from the other
    t0 = bytesToWord( InvSbox[byte0(s0)],
                          InvSbox[byte1(s3)],
                          InvSbox[byte2(s2)],
                          InvSbox[byte3(s1)]) ^ local_pointer[4];
    t1 = bytesToWord( InvSbox[byte0(s1)],
                          InvSbox[byte1(s0)],
                          InvSbox[byte2(s3)],
                          InvSbox[byte3(s2)]) ^ local_pointer[5];
    t2 = bytesToWord( InvSbox[byte0(s2)],
                          InvSbox[byte1(s1)],
                          InvSbox[byte2(s0)],
                          InvSbox[byte3(s3)]) ^ local_pointer[6];
    t3 = bytesToWord( InvSbox[byte0(s3)],
                          InvSbox[byte1(s2)],
                          InvSbox[byte2(s1)],
                          InvSbox[byte3(s0)]) ^ local_pointer[7];
    dest[0] = t0;
    dest[1] = t1;
    dest[2] = t2;
    dest[3] = t3;
}
