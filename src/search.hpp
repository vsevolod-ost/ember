// search.hpp — linear search in the box (Lab 4, theory §3).
#pragma once

#include <cstdint>

#include "memory.hpp"

// The first address in [lo, hi) that holds `needle`; hi if there is none.
std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle);
