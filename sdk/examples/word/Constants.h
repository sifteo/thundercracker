#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "config.h"

const unsigned MAX_LETTERS_PER_CUBE = 3;
const unsigned MAX_LETTERS_PER_WORD = MAX_LETTERS_PER_CUBE * NUM_CUBES;// TODO longer words post CES: _SYS_NUM_CUBE_SLOTS * GameStateMachine::getCurrentMaxLettersPerCube();

#endif // CONSTANTS_H
