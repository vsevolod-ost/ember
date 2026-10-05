// search.hpp — linear search in the box (Lab 4, theory §3).
#pragma once

#include <cstdint>

#include "memory.hpp"

// The first address in [lo, hi) that holds `needle`; hi if there is none.
std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle);

// The first address in [lo, hi) that does NOT hold `needle`; hi if there is
// none. find_not(mem, VRAM_LO, VRAM_HI + 1, 0) is the first non-zero byte of
// the screen (Lab 5, M5).
std::uint16_t find_not(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle);
