// hexdump.cpp — one row printer shared by the host dump and the guest range
// dump. The row printer never looks at Memory: the guest dump copies a row
// out through mem_get first, so mem_get/mem_set stay the only code that
// touches Memory::data.
#include "hexdump.hpp"

#include <iomanip>
#include <iostream>

static const std::size_t ROW = 16;

static bool is_printable(Byte b) { return b >= 0x20 && b <= 0x7E; }

// `label` in the left column, then up to 16 bytes in hex, then the ASCII
// gutter. A short last row is padded so the gutter still lines up.
static void print_row(std::size_t label, const Byte* bytes, std::size_t count) {
    std::cout << std::hex << std::setfill('0') << std::setw(4) << label << "  ";
    for (std::size_t col = 0; col < ROW; ++col) {
        if (col < count)
            std::cout << std::setw(2) << static_cast<int>(bytes[col]) << ' ';
        else
            std::cout << "   ";
    }
    std::cout << " |";
    for (std::size_t col = 0; col < count; ++col)
        std::cout << (is_printable(bytes[col]) ? static_cast<char>(bytes[col]) : '.');
    std::cout << "|\n" << std::dec << std::setfill(' ');
}

void dump(const void* ptr, std::size_t n) {
    // A void* cannot be dereferenced or stepped: it has no element size.
    // Looking at it as bytes gives it one — and reading any object as
    // unsigned bytes is allowed by C++.
    const Byte* p = static_cast<const Byte*>(ptr);
    for (std::size_t off = 0; off < n; off += ROW)
        print_row(off, p + off, n - off < ROW ? n - off : ROW);
}

void dump_range(const Memory& mem, std::size_t from, std::size_t n) {
    if (from >= MEM_SIZE || n == 0)
        return;
    std::size_t end = (n > MEM_SIZE - from) ? MEM_SIZE : from + n;
    for (std::size_t row = from - from % ROW; row < end; row += ROW) {
        Byte bytes[ROW];
        for (std::size_t col = 0; col < ROW; ++col)
            bytes[col] = mem_get(mem, row + col);
        print_row(row, bytes, ROW);
    }
}
