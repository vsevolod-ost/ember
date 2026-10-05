// heap.hpp — the guest heap, HEAP_LO..HEAP_HI (Lab 6).
//
// ALLOC (ISA 0x60) is a bump allocator: a fresh block starts at heap_ptr and
// heap_ptr moves up past it, so two blocks in a row have no gap between them.
// FREE (own 0x72) gives a block back. The guest says how big it was (A, the
// size it asked ALLOC for): there is no header before a block to keep it in.
//
// Freed blocks form a list in GUEST memory, inside the blocks themselves: the
// first 3 bytes of a free block are a node, the address of the next free
// block (16 bits, low byte first, 0 = end) and the size of this one. The head
// is CPU::free_head. A program that never uses FREE sees a pure bump.
#pragma once

#include <cstdint>

#include "cpu.hpp"

// Bytes of a free-list node: next (2) + size (1). A smaller block can only
// be freed when it is the top one (it just lowers heap_ptr).
const std::uint16_t NODE_SIZE = 3;

enum class HeapResult {
    ok,
    no_room,      // ALLOC: the block does not fit below HEAP_HI + 1; nothing done
    bad_block,    // FREE: size 0, or [at, at + n) is not inside HEAP_LO..heap_ptr
    already_free, // FREE: [at, at + n) overlaps a block on the free list (double free)
    too_small,    // FREE: not the top block and n < NODE_SIZE: no room for a node
    broken_list,  // a node holds an impossible address or size: the guest wrote
                  // into a freed block. Nothing done
};

// A block of n bytes: first fit from the free list (a node of exactly n
// bytes, or the tail of one at least n + NODE_SIZE big), else bump. `at` is
// the block, `reused` says where it came from. n == 0 gives at = heap_ptr and
// moves nothing. Nothing changes, `at` included, unless ok.
HeapResult heap_alloc(CPU& cpu, Byte n, std::uint16_t& at, bool& reused);

// Give [at, at + n) back. The top block (it ends at heap_ptr) lowers heap_ptr,
// and with it any free blocks that now end there; `top` says so. Any other
// block becomes a node at the head of the list. Nothing changes unless ok.
HeapResult heap_free(CPU& cpu, std::uint16_t at, Byte n, bool& top);

// Walk the free list: false if a node is impossible or comes round a second
// time (a loop); `bad` = its address.
bool heap_check(const CPU& cpu, std::uint16_t& bad);

// heap_ptr, how much is used and left, and the free list.
void heap_print(const CPU& cpu);
