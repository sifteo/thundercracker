#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "config.h"

const unsigned MAX_LETTERS_PER_CUBE = 3;
const unsigned MAX_LETTERS_PER_WORD = MAX_LETTERS_PER_CUBE * NUM_CUBES;// TODO longer words post CES: _SYS_NUM_CUBE_SLOTS * GameStateMachine::getCurrentMaxLettersPerCube();
const float TRANSITION_ANIM_LENGTH = 1.7f;

#endif // CONSTANTS_H
