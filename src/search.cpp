// search.cpp — GIVEN (Lab 4, theory §3), pasted as is. Reads only through
// mem_get, so it never touches Memory::data itself.
#include "search.hpp"

// знайти перше i в [lo, hi), де mem_get(mem, i) == needle; якщо промах — повернути hi
std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) == needle)
            return i;
    }
    return hi;
}
