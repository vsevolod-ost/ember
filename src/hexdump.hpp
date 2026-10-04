// hexdump.hpp — two more ways of looking at bytes, in the same 16-per-line
// format as the given dump().
#pragma once

#include <cstddef>

#include "memory.hpp"

// Host: any object of this process, byte by byte. `void*` because the
// function does not care WHAT is there, only how many bytes: dump(&cpu,
// sizeof(cpu)). The left column is the offset from `ptr`, not an address.
void dump(const void* ptr, std::size_t n);

// Guest: `n` bytes of ember memory from `from`, read through mem_get. Rows
// are aligned to 16 like the full dump; the range is clipped at MEM_SIZE.
void dump_range(const Memory& mem, std::size_t from, std::size_t n);
