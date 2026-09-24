// opcodes.hpp — opcode bytes, copied from the ISA table. Never renumber:
// the table is the contract, not this file.
#pragma once

#include <cstdint>

enum class Op : std::uint8_t {
    // 0x0_ — control
    Halt = 0x00,
    Nop = 0x01,
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
    // 0x2_ — data movement (Lab 3 rows, needed by checks/lab-02.txt)
    LoadiA = 0x20,
    LoadiB = 0x21,
};
