// sort.cpp — bubble sort and insertion sort over a range of guest memory.
//
// No swap(Byte&, Byte&) here: a reference into Memory::data would touch the
// box past mem_get/mem_set. A swap is a read of both cells and two writes.
#include "sort.hpp"

#include <iostream>

#include "display.hpp"

static void swap_cells(Memory& mem, std::size_t i, std::size_t j) {
    const Byte tmp = mem_get(mem, i);
    mem_set(mem, i, mem_get(mem, j));
    mem_set(mem, j, tmp);
}

// mem[lo + i] as column i, growing up from the bottom row: n columns of width
// 64 / n (the last pixel of a column of width >= 2 is a gap), height
// (v + 7) / 8 pixels, so 0 is empty, 1..8 is one pixel and 255 is all 32.
static void draw_columns(Memory& mem, std::size_t lo, std::size_t hi) {
    const int n = static_cast<int>(hi - lo);
    const int w = SCREEN_W / n;
    const int gap = w >= 2 ? 1 : 0;
    clear(mem);
    for (int i = 0; i < n; ++i) {
        const int h = (mem_get(mem, lo + i) + 7) / 8;
        for (int x = i * w; x < i * w + w - gap; ++x) {
            for (int y = SCREEN_H - 1; y >= SCREEN_H - h; --y)
                plot(mem, x, y);
        }
    }
}

// The title line, then the columns and the given show().
static void draw_frame(Memory& mem, std::size_t lo, std::size_t hi, unsigned pass, unsigned swaps) {
    if (pass == 0)
        std::cout << "before\n";
    else
        std::cout << "pass " << pass << ": " << swaps << " swaps\n";
    draw_columns(mem, lo, hi);
    show(mem);
}

static void bubble(Memory& mem, std::size_t lo, std::size_t hi, bool draw, SortStats& s) {
    // After pass k the k largest bytes are at the end: each pass looks at one less.
    for (std::size_t end = hi; end > lo + 1; --end) {
        unsigned swaps = 0;
        ++s.passes;
        for (std::size_t i = lo; i + 1 < end; ++i) {
            ++s.compares;
            if (mem_get(mem, i) > mem_get(mem, i + 1)) {
                swap_cells(mem, i, i + 1);
                ++swaps;
            }
        }
        s.swaps += swaps;
        if (draw)
            draw_frame(mem, lo, hi, s.passes, swaps);
        if (swaps == 0)
            break; // nothing moved: already in order
    }
}

static void insert(Memory& mem, std::size_t lo, std::size_t hi, bool draw, SortStats& s) {
    // [lo, i) is sorted; mem[i] walks left while its left neighbour is bigger.
    for (std::size_t i = lo + 1; i < hi; ++i) {
        unsigned swaps = 0;
        ++s.passes;
        for (std::size_t j = i; j > lo; --j) {
            ++s.compares;
            if (mem_get(mem, j - 1) <= mem_get(mem, j))
                break;
            swap_cells(mem, j - 1, j);
            ++swaps;
        }
        s.swaps += swaps;
        if (draw)
            draw_frame(mem, lo, hi, s.passes, swaps);
    }
}

SortStats sort_range(Memory& mem, std::size_t lo, std::size_t hi, SortKind kind, bool draw) {
    SortStats s;
    if (draw)
        draw_frame(mem, lo, hi, 0, 0);
    if (kind == SortKind::bubble)
        bubble(mem, lo, hi, draw, s);
    else
        insert(mem, lo, hi, draw, s);
    return s;
}
