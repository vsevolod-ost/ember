// display.hpp — the 64x32 screen (Lab 5). There is no pixel array on the
// host: the screen IS the 256 bytes VRAM_LO..VRAM_HI of guest memory, one bit
// per pixel, 8 bytes per row, the leftmost pixel of a byte in bit 7.
#pragma once

#include "memory.hpp"

const int SCREEN_W = 64, SCREEN_H = 32;

// GIVEN: print the screen, 32 lines of 64 '#' (lit) or '.' (dark).
void show(const Memory& mem);

// ВАШЕ. Та сама формула навпаки: не читати біт, а виставити його.
// Відхилити x >= 64 або y >= 32.
// Returns false and changes nothing for a pixel off the screen (negative x or
// y too), so PLOT can set C and the REPL can say why.
bool plot(Memory& mem, int x, int y);
void clear(Memory& mem); // 256 байтів нулів

// The same pixel turned off (UNPLOT, *opt*); false off the screen, like plot.
bool unplot(Memory& mem, int x, int y);

// Is the pixel lit? false off the screen.
bool pixel(const Memory& mem, int x, int y);

// The border of the screen, drawn with plot; the inside is left as it is.
void frame(Memory& mem);

// The leftmost lit pixel in row y (0..31); -1 if the row is dark.
int rowmin(const Memory& mem, int y);
