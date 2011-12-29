#include <sifteo.h>
#include "PartialAnimationData.h"

const static EyeData letterEyeData[264] =
{
    // Upper-left X, Y left eye,     Upper-left X, Yright eye,   true if this letter needs a special eyeball animation
    {0, 64,     112, 64,    false},  // A
    {0, 0,      0, 0,       false},  // B
    {0, 0,      0, 0,       false},  // C
    {0, 0,      0, 0,       false},  // D
    {0, 64,     112, 64,    false},  // E
    {0, 0,      0, 0,       false},  // F
    {0, 0,      0, 0,       false},  // G
    {0, 0,      0, 0,       false},  // H
    {0, 64,     112, 64,    false},  // I
    {0, 0,      0, 0,       false},  // J
    {0, 0,      0, 0,       false},  // K
    {0, 0,      0, 0,       false},  // L
    {0, 0,      0, 0,       false},  // M
    {0, 0,      0, 0,       false},  // N
    {0, 64,     112, 64,    false},  // O
    {0, 0,      0, 0,       false},  // P
    {0, 0,      0, 0,       false},  // Q
    {0, 0,      0, 0,       false},  // R
    {0, 0,      0, 0,       false},  // S
    {0, 0,      0, 0,       false},  // T
    {0, 64,     112, 64,    false},  // U
    {0, 0,      0, 0,       false},  // V
    {0, 0,      0, 0,       false},  // W
    {0, 0,      0, 0,       false},  // X
    {0, 0,      0, 0,       false},  // Y
    {0, 0,      0, 0,       false},  // Z
};


const EyeData& getEyeData(char letter)
{
    unsigned i = letter - 'A';
    ASSERT(i < arraysize(letterEyeData));
    return letterEyeData[i];
}
