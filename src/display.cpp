// display.cpp — the screen in guest memory. The given show() reads
// Memory::data directly, like the given dump() (the only two places outside
// memory.cpp that do); everything written here goes through mem_get/mem_set.
//
// One formula everywhere (ISA §7): pixel (x, y) is bit 7 - x % 8 of the byte
// at VRAM_LO + y * 8 + x / 8.
#include "display.hpp"

#include <iostream>

// ДАНО. Намалювати відеопам'ять у терміналі.
void show(const Memory& mem) {
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            Byte cell = mem.data[0xA00 + y * 8 + (x / 8)];
            bool lit = (cell >> (7 - (x % 8))) & 1;
            std::cout << (lit ? '#' : '.');
        }
        std::cout << '\n';
    }
}

static bool on_screen(int x, int y) { return x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H; }

bool plot(Memory& mem, int x, int y) {
    if (!on_screen(x, y))
        return false; // checked before the write: a refused plot changes nothing
    const std::size_t addr = VRAM_LO + y * 8 + x / 8;
    const Byte mask = static_cast<Byte>(1u << (7 - x % 8));
    mem_set(mem, addr, static_cast<Byte>(mem_get(mem, addr) | mask)); // the other 7 pixels stay
    return true;
}

bool unplot(Memory& mem, int x, int y) {
    if (!on_screen(x, y))
        return false;
    const std::size_t addr = VRAM_LO + y * 8 + x / 8;
    const Byte mask = static_cast<Byte>(1u << (7 - x % 8));
    mem_set(mem, addr, static_cast<Byte>(mem_get(mem, addr) & ~mask));
    return true;
}

void clear(Memory& mem) {
    for (std::size_t i = 0; i < 256; ++i)
        mem_set(mem, VRAM_LO + i, 0);
}

bool pixel(const Memory& mem, int x, int y) {
    if (!on_screen(x, y))
        return false;
    return (mem_get(mem, VRAM_LO + y * 8 + x / 8) >> (7 - x % 8)) & 1;
}

void frame(Memory& mem) {
    for (int x = 0; x < SCREEN_W; ++x) {
        plot(mem, x, 0);
        plot(mem, x, SCREEN_H - 1);
    }
    for (int y = 0; y < SCREEN_H; ++y) {
        plot(mem, 0, y);
        plot(mem, SCREEN_W - 1, y);
    }
}

int rowmin(const Memory& mem, int y) {
    for (int x = 0; x < SCREEN_W; ++x) {
        if (pixel(mem, x, y))
            return x;
    }
    return -1;
}
