// sprite.hpp — sprites on the HOST (Lab 6): a record per moving dot, an
// array of eight, and the screen as a picture of that array.
#pragma once

#include <cstdint>

#include "memory.hpp"

// GIVEN (theory §1). Five one-byte fields, so no padding: sizeof == 5.
struct Sprite {
    std::uint8_t x, y;
    std::int8_t vx, vy;
    bool alive;
};

const int SPRITE_COUNT = 8; // Sprite sprites[SPRITE_COUNT] in main

// One tick of one sprite: x += vx, y += vy, bouncing off the edges of the
// 64x32 screen. Speeds are at most 63 (x) and 31 (y), so one bounce is
// always enough. On an edge the speed always points back inside.
void sprite_move(Sprite& s);

// sprite_move every alive sprite.
void sprites_tick(Sprite sprites[]);

// The screen as a picture of the array: clear VRAM, plot every alive sprite.
void sprites_draw(Memory& mem, const Sprite sprites[]);

// How many sprites are alive.
int sprites_alive(const Sprite sprites[]);
