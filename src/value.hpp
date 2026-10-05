// value.hpp — one 16-bit number that says what it is (Lab 6, *opt*).
//
// A union would put a byte, an address and a sprite index in the same bytes,
// and the PROGRAMMER would have to remember which one is there. A tagged
// struct keeps that knowledge in the bytes: `kind` says how to read `bits`.
#pragma once

#include <cstdint>

#include "memory.hpp"
#include "sprite.hpp"

// lab: struct Value { enum class Kind { Byte, Addr, SpriteId }; uint16_t bits; };
// That line declares the type Kind but no member of it: sizeof == 2 and no
// tag in the bytes. `kind` is the tag; ": std::uint8_t" keeps it one byte
// (an int would make sizeof 8).
struct Value {
    enum class Kind : std::uint8_t { Byte, Addr, SpriteId };
    Kind kind;          // the tag: which of the three `bits` means
    std::uint16_t bits; // a byte (0..255), a guest address or a sprite index
};

// A value of this kind, if `n` is in its range (Byte 0..255, Addr
// 0..MEM_SIZE-1, SpriteId 0..SPRITE_COUNT-1); false and `out` untouched if not.
bool make_value(Value::Kind kind, long n, Value& out);

// Why make_value refused `n`, as one line.
void print_value_error(Value::Kind kind, long n);

// One line, chosen by the tag: a byte four ways, an address with its region
// of the memory map and the byte there, a sprite's record.
void print_value(const Value& v, const Memory& mem, const Sprite sprites[]);
