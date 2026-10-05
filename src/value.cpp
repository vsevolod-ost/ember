// value.cpp — make and print a tagged Value: one switch on the tag each.
#include "value.hpp"

#include <iostream>

#include "cpu.hpp"
#include "dump.hpp"

bool make_value(Value::Kind kind, long n, Value& out) {
    long hi = 0;
    switch (kind) {
    case Value::Kind::Byte:
        hi = 255;
        break;
    case Value::Kind::Addr:
        hi = static_cast<long>(MEM_SIZE) - 1;
        break;
    case Value::Kind::SpriteId:
        hi = SPRITE_COUNT - 1;
        break;
    }
    if (n < 0 || n > hi)
        return false;
    out = {kind, static_cast<std::uint16_t>(n)};
    return true;
}

void print_value_error(Value::Kind kind, long n) {
    switch (kind) {
    case Value::Kind::Byte:
        std::cout << "a byte is 0..255, got " << n << '\n';
        break;
    case Value::Kind::Addr:
        std::cout << "address " << n << " is outside 0.." << MEM_SIZE - 1 << '\n';
        break;
    case Value::Kind::SpriteId:
        std::cout << "sprite index is 0.." << SPRITE_COUNT - 1 << ", got " << n << '\n';
        break;
    }
}

// The region of the memory map (ISA §2) that holds `addr`.
static const char* region(std::size_t addr) {
    if (addr <= CODE_HI)
        return "code";
    if (addr <= DATA_HI)
        return "data";
    if (addr <= VRAM_HI)
        return "vram";
    if (addr == PORT_OUT)
        return "port_out";
    if (addr == TICKS)
        return "ticks";
    if (addr == STATUS_ADDR)
        return "status";
    if (addr < HEAP_LO)
        return "spare";
    if (addr <= HEAP_HI)
        return "heap";
    return "stack";
}

void print_value(const Value& v, const Memory& mem, const Sprite sprites[]) {
    switch (v.kind) {
    case Value::Kind::Byte:
        std::cout << "byte " << v.bits << ": ";
        show_byte(static_cast<Byte>(v.bits));
        break;
    case Value::Kind::Addr:
        std::cout << "addr " << hex(v.bits, 4) << ": " << region(v.bits) << ", byte "
                  << hex(mem_get(mem, v.bits), 2) << '\n';
        break;
    case Value::Kind::SpriteId: {
        // int8_t is a char to std::cout: print the number, not the character.
        const Sprite& s = sprites[v.bits];
        std::cout << "sprite " << v.bits << ": x=" << static_cast<int>(s.x)
                  << " y=" << static_cast<int>(s.y) << " vx=" << static_cast<int>(s.vx)
                  << " vy=" << static_cast<int>(s.vy) << (s.alive ? " alive" : " dead") << '\n';
        break;
    }
    }
}
