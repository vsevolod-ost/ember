// sort.hpp — `sort`: bytes of guest memory sorted in place (Lab 5, Advanced).
#pragma once

#include <cstddef>

#include "memory.hpp"

enum class SortKind {
    bubble, // passes over the range, swapping neighbours; stops after a pass with no swap
    insert, // each byte in turn moved left by swapping neighbours until it fits
};

struct SortStats {
    unsigned passes = 0, compares = 0, swaps = 0;
};

// Sort [lo, hi) ascending as unsigned bytes, through mem_get/mem_set only.
// The caller checks 0 <= lo <= hi <= MEM_SIZE. With `draw`, the range is
// drawn as columns on the screen (which is overwritten) and shown before the
// first pass and after every pass; the caller checks 1 <= hi - lo <= 64 and
// that the range does not overlap VRAM.
SortStats sort_range(Memory& mem, std::size_t lo, std::size_t hi, SortKind kind, bool draw);
