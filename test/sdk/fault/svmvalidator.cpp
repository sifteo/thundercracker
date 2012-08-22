/*
 * Who needs inline assembly, we have inline machine code.
 *
 * Secret decoder ring:
 *   arm-none-eabi-objdump -D -M force-thumb test-fault.elf | less
 */

#include "fault.h"

CODE_BLOCK(testReturnOnly) {
    0xdf00,     // svc 0 (return)
    0xffff,     // invalid
    0xffff,     // invalid
    0xffff,     // invalid
}; 

CODE_BLOCK(testNoTerminator) {
    // Fill the whole block with mov instructions. No terminator at all.
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
    0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 0x2600, 
}; 

CODE_BLOCK(testInfLoop) {
    // This will fault immediately with a bad store, but from the validator's
    // perspective this is a small infinite loop with no terminators.
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-6 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testB1) {
    // Branch target underflow by one halfword
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-7 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testB2) {
    // Branch target underflow by one bundle
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-8 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testB3) {
    // Branch to self
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-2 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testB4) {
    // Branch to instruction after self
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-1 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testB5) {
    // Branch to bundle after self
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (0 & 0x7FF)    // b        <block origin>
};

CODE_BLOCK(testBCC1) {
    // Conditional branch target underflow by one instruction
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xd000 | (-7 & 0xFF),   // beq      <underflow>
    0xe000 | (-7 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testBCC2) {
    // Conditional branch target underflow by one bundle
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xd000 | (-8 & 0xFF),   // beq      <underflow>
    0xe000 | (-7 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testBCC3) {
    // Conditional branch to self
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xd000 | (-2 & 0xFF),   // beq      <self>
    0xe000 | (-7 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testBCC4) {
    // Conditional branch to next instruction
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xd000 | (-1 & 0xFF),   // beq      <next instr>
    0xe000 | (-7 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testBCC5) {
    // Conditional branch to next bundle
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xd000 | ( 0 & 0xFF),   // beq      <next bundle>
    0xe000 | (-7 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testCBZ1) {
    // Conditional branch to first possible bundle
    0xb100,                 // cbz      r0, <next bundle>
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0x2000,                 // mov      r0, #0  
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-8 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testCBZ2) {
    // Conditional branch to last valid bundle
    0xb920,                 // cbnz     r0, <final branch>
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0x2000,                 // mov      r0, #0  
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-8 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testCBZ3) {
    // Not bundle aligned
    0xb918,                 // cbnz     r0, <final branch>
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0x2000,                 // mov      r0, #0  
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-8 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testCBZ4) {
    // Also not bundle aligned
    0xb928,                 // cbnz     r0, <final branch>
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0x2000,                 // mov      r0, #0  
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-8 & 0x7FF)   // b        <block origin>
};

CODE_BLOCK(testCBZ5) {
    // Branch to invalid instr
    0xb930,                 // cbnz     r0, <final branch>
    0x2000,                 // mov      r0, #0
    0xdfe0,                 // svc      0xe0
    0x2000,                 // mov      r0, #0  
    0xf8c9, 0x0000,         // str.w    r0, [r9]
    0xe000 | (-8 & 0x7FF),  // b        <block origin>
    0xffff,                 // invalid
};

void testSvmValidator()
{
    // Plain call, always works
    call(testReturnOnly);
    SCRIPT(LUA, assertFault(nil));

    // Reserved LSBs, currently have no effect. Function still returns.
    call(testReturnOnly, 1);
    SCRIPT(LUA, assertFault(nil));
    call(testReturnOnly, 2);
    SCRIPT(LUA, assertFault(nil));
    call(testReturnOnly, 3);
    SCRIPT(LUA, assertFault(nil));

    // Jumping to invalid bundle
    call(testReturnOnly, 4);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));

    // No terminator. There's no safe place to jump here.
    for (unsigned i = 0; i < 256; ++i) {
        call(testNoTerminator, i);
        SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    }

    // Infinite loop with no terminators (runtime-only store fault)
    for (unsigned i = 0*4; i < 1*4; ++i) {
        // First bundle: Store address fault
        call(testInfLoop, i);
        SCRIPT(LUA, assertFault(F_STORE_ADDRESS));
    }
    for (unsigned i = 2*4; i < 3*4; ++i) {
        // Bundle 2 loops back to the beginning
        call(testInfLoop, i);
        SCRIPT(LUA, assertFault(F_STORE_ADDRESS));
    }
    for (unsigned i = 3*4; i < 256; ++i) {
        // Bundle 3 and later are invalid
        call(testInfLoop, i);
        SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    }

    // Branch underflow
    call(testB1, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    call(testB2, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));

    // Branch to self- okay
    call(testB3, 0);
    SCRIPT(LUA, assertFault(F_STORE_ADDRESS));

    // Forward branch (DAG not closed)
    call(testB4, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    call(testB5, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));

    // Conditional branch underflow
    call(testBCC1, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    call(testBCC2, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));

    // Conditional branch to self
    call(testBCC3, 0);
    SCRIPT(LUA, assertFault(F_STORE_ADDRESS));

    // Forward conditional branch (DAG not closed)
    call(testBCC4, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    call(testBCC5, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));

    // Well-formed CBZ
    call(testCBZ1, 0);
    SCRIPT(LUA, assertFault(F_STORE_ADDRESS));
    call(testCBZ2, 0);
    SCRIPT(LUA, assertFault(F_STORE_ADDRESS));

    // Bad CBZ target
    call(testCBZ3, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    call(testCBZ4, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
    call(testCBZ5, 0);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
}
