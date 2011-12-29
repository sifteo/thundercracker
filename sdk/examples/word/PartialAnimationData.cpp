#include <sifteo.h>
#include "PartialAnimationData.h"

const static EyeData letterEyeData[264] =
{
    // Upper-left X, Y left eye,     Upper-left X, Yright eye,   true if this letter needs a special eyeball animation
    {64, 24,     80, 24,       false},  // A
    {64, 24,     80, 24,       false},  // B
    {64, 24,     80, 24,       false},  // C
    {64, 24,     80, 24,       false},  // D
    {64, 24,     80, 24,       false},  // E
    {64, 24,     80, 24,       false},  // F
    {64, 24,     80, 24,       false},  // G
    {64, 24,     80, 24,       false},  // H
    {64, 24,     80, 24,       false},  // I
    {64, 24,     80, 24,       false},  // J
    {64, 24,     80, 24,       false},  // K
    {32, 24,     48, 24,       false},  // L
    {64, 24,     80, 24,       false},  // M
    {64, 24,     80, 24,       false},  // N
    {64, 24,     80, 24,       false},  // O
    {64, 24,     80, 24,       false},  // P
    {64, 24,     80, 24,       false},  // Q
    {64, 24,     80, 24,       false},  // R
    {64, 24,     80, 24,       false},  // S
    {64, 24,     80, 24,       false},  // T
    {64, 24,     80, 24,       false},  // U
    {88, 24,     104, 24,      false},  // V
    {88, 24,     104, 24,      false},  // W
    {64, 24,     80, 24,       false},  // X
    {64, 24,     80, 24,       false},  // Y
    {64, 24,     80, 24,       false},  // Z
};


const EyeData& getEyeData(char letter)
{
    unsigned i = letter - 'A';
    ASSERT(i < arraysize(letterEyeData));
    return letterEyeData[i];
}
