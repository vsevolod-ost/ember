// opcodes.hpp — opcode bytes, copied from the ISA table. Never renumber:
// the table is the contract, not this file.
#pragma once

#include <cstdint>

enum class Op : std::uint8_t {
    // 0x0_ — control and output
    Halt = 0x00,
    Nop = 0x01,
    Out = 0x02,  // print A as a character
    Outn = 0x03, // print A as a decimal number and a space
    // 0x1_ — ALU
    Add = 0x10,
    Sub = 0x11,
    And = 0x12,
    Or = 0x13,
    Xor = 0x14,
    Not = 0x15,
    Shl = 0x16,
    Shr = 0x17,
    Inc = 0x18,
    Dec = 0x19,
    Cmp = 0x1A, // A - B for the flags only; A is kept (Lab 4)
    // 0x2_ — data movement (Lab 3 rows; HLOW is *opt*, Lab 4, done early)
    LoadiA = 0x20,
    LoadiB = 0x21,
    LoadA = 0x22,   // LOAD A, [addr16]
    LoadB = 0x23,   // LOAD B, [addr16]
    StoreA = 0x24,  // STORE [addr16], A
    StoreB = 0x25,  // STORE [addr16], B
    MovAB = 0x26,   // MOV A, B   (A = B)
    MovBA = 0x27,   // MOV B, A   (B = A)
    LoadH = 0x28,   // LOADH H, imm16
    LoadAH = 0x29,  // LOAD A, [H]
    StoreHA = 0x2A, // STORE [H], A
    IncH = 0x2B,
    DecH = 0x2C,
    HLow = 0x2D, // A = H & 0xFF
    // 0x3_ — jumps, operand addr16 (Lab 4; JC/JNC are *opt*)
    Jmp = 0x30,
    Jz = 0x31,
    Jnz = 0x32,
    Jc = 0x33,
    Jnc = 0x34,
    // 0x7_ — own extensions (ISA §8: 0x70-0xFF are free), Lab 4
    Jg = 0x70, // jump if A > B as signed bytes (-128..127); flags untouched
    Jl = 0x71, // jump if A < B as signed bytes (-128..127); flags untouched
};
