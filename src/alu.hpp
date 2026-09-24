// alu.hpp — arithmetic and logic on 8-bit values.
#pragma once

#include "memory.hpp"

// Z: result is 0. N: bit 7 of the result is 1. C: carry/borrow/shifted-out bit.
struct Flags {
    bool z, n, c;
};

// Each op returns the 8-bit result and updates only the flags the ISA table
// lists for it; the others keep their old value.
Byte alu_add(Byte a, Byte b, Flags& f); // Z N C
Byte alu_sub(Byte a, Byte b, Flags& f); // Z N C, C = borrow
Byte alu_and(Byte a, Byte b, Flags& f); // Z N, C=0
Byte alu_or(Byte a, Byte b, Flags& f);  // Z N, C=0
Byte alu_xor(Byte a, Byte b, Flags& f); // Z N, C=0
Byte alu_not(Byte a, Flags& f);         // Z N, C=0
Byte alu_shl(Byte a, Flags& f);         // Z N C, C = old bit 7
Byte alu_shr(Byte a, Flags& f);         // Z N C, C = old bit 0
Byte alu_inc(Byte a, Flags& f);         // Z N
Byte alu_dec(Byte a, Flags& f);         // Z N
