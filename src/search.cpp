// search.cpp — find is GIVEN (Lab 4, theory §3), pasted as is; find_not is ours.
// Both read only through mem_get, never Memory::data itself.
#include "search.hpp"

// знайти перше i в [lo, hi), де mem_get(mem, i) == needle; якщо промах — повернути hi
std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) == needle)
            return i;
    }
    return hi;
}

// Ours (Lab 5): the same loop with != instead of ==. Two questions ("where is
// the first L?", "where does the picture start?"), one loop.
std::uint16_t find_not(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) != needle)
            return i;
    }
    return hi;
}
