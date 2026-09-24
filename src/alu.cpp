// alu.cpp — the ALU. Results are computed in int by C++'s promotion rules;
// the cast back to Byte keeps the low 8 bits, which is the 8-bit wrap.
#include "alu.hpp"

#include <cassert>

static void set_zn(Byte r, Flags& f) {
    f.z = r == 0;
    f.n = (r & 0x80) != 0;
}

// Ripple-carry adder, one bit at a time: sum bit = a ^ b ^ carry,
// carry out = at least two of the three are 1.
Byte alu_add(Byte a, Byte b, Flags& f) {
    Byte sum = 0;
    bool carry = false;
    for (int i = 0; i < 8; ++i) {
        const bool ab = (a >> i) & 1;
        const bool bb = (b >> i) & 1;
        if (ab ^ bb ^ carry)
            sum = static_cast<Byte>(sum | (1u << i));
        carry = (ab && bb) || (ab && carry) || (bb && carry);
    }
    assert(sum == static_cast<Byte>(a + b));
    assert(carry == (a + b > 0xFF));
    f.c = carry;
    set_zn(sum, f);
    return sum;
}

Byte alu_sub(Byte a, Byte b, Flags& f) {
    const Byte r = static_cast<Byte>(a - b);
    f.c = a < b;
    set_zn(r, f);
    return r;
}

Byte alu_and(Byte a, Byte b, Flags& f) {
    const Byte r = a & b;
    f.c = false;
    set_zn(r, f);
    return r;
}

Byte alu_or(Byte a, Byte b, Flags& f) {
    const Byte r = a | b;
    f.c = false;
    set_zn(r, f);
    return r;
}

Byte alu_xor(Byte a, Byte b, Flags& f) {
    const Byte r = a ^ b;
    f.c = false;
    set_zn(r, f);
    return r;
}

Byte alu_not(Byte a, Flags& f) {
    // Without the cast ~a is an int with 24 extra one-bits on top.
    const Byte r = static_cast<Byte>(~a);
    f.c = false;
    set_zn(r, f);
    return r;
}

Byte alu_shl(Byte a, Flags& f) {
    const Byte r = static_cast<Byte>(a << 1);
    f.c = (a & 0x80) != 0;
    set_zn(r, f);
    return r;
}

Byte alu_shr(Byte a, Flags& f) {
    const Byte r = a >> 1;
    f.c = (a & 0x01) != 0;
    set_zn(r, f);
    return r;
}

Byte alu_inc(Byte a, Flags& f) {
    const Byte r = static_cast<Byte>(a + 1);
    set_zn(r, f);
    return r;
}

Byte alu_dec(Byte a, Flags& f) {
    const Byte r = static_cast<Byte>(a - 1);
    set_zn(r, f);
    return r;
}
