#include <sifteo.h>
#include "PartialAnimationData.h"

const static EyeData letterEyeData[27] =
{
    // Tile coordinate legend
    //
    // Each 8x8 px square is a "tile".
    // Each tile has a coordinate, like this:
    //
    // 0,0  1,0 ...             15,0
    // 0,1  1,1 ...             15,1
    // ...
    // 0,15  1,15 ...           15,15


    // Upper-left tile coordinate left eye,     Upper-left tile coordinate right eye,   true if this letter needs a special eyeball animation
    {0, 0,      0, 0,       false},  // A
    {0, 0,      0, 0,       false},  // B
    {0, 0,      0, 0,       false},  // C
    {0, 0,      0, 0,       false},  // D
    {0, 0,      0, 0,       false},  // E
    {0, 0,      0, 0,       false},  // F
    {0, 0,      0, 0,       false},  // G
    {0, 0,      0, 0,       false},  // H
    {0, 0,      0, 0,       false},  // I
    {0, 0,      0, 0,       false},  // J
    {0, 0,      0, 0,       false},  // K
    {0, 0,      0, 0,       false},  // L
    {0, 0,      0, 0,       false},  // M
    {0, 0,      0, 0,       false},  // N
    {0, 0,      0, 0,       false},  // O
    {0, 0,      0, 0,       false},  // P
    {0, 0,      0, 0,       false},  // Q
    {0, 0,      0, 0,       false},  // R
    {0, 0,      0, 0,       false},  // S
    {0, 0,      0, 0,       false},  // T
    {0, 0,      0, 0,       false},  // U
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
