// heap.cpp — ALLOC and FREE over HEAP_LO..HEAP_HI. The nodes of the free list
// are guest memory: every byte of them is read and written through
// guest_read/guest_write, like any other guest access, and every number read
// from them is checked before it is believed.
#include "heap.hpp"

#include <iostream>

static std::uint16_t node_next(const CPU& cpu, std::uint16_t node) {
    Byte lo = 0, hi = 0;
    guest_read(cpu, node, lo);
    guest_read(cpu, node + 1u, hi);
    return static_cast<std::uint16_t>(lo | (hi << 8));
}

static Byte node_size(const CPU& cpu, std::uint16_t node) {
    Byte size = 0;
    guest_read(cpu, node + 2u, size);
    return size;
}

static void set_next(CPU& cpu, std::uint16_t node, std::uint16_t next) {
    guest_write(cpu, node, static_cast<Byte>(next & 0xFF));
    guest_write(cpu, node + 1u, static_cast<Byte>(next >> 8));
}

static void set_size(CPU& cpu, std::uint16_t node, Byte size) { guest_write(cpu, node + 2u, size); }

// Could a node be at `node`? It must lie in the used part of the heap, and
// so must the block its size describes.
static bool node_ok(const CPU& cpu, std::uint16_t node) {
    if (node < HEAP_LO || node + NODE_SIZE > cpu.heap_ptr)
        return false;
    const Byte size = node_size(cpu, node);
    return size >= NODE_SIZE && node + size <= cpu.heap_ptr;
}

// Follow the list from the head to the 0 at its end; with `print`, print
// each node on the way. A node that is impossible, or that was already seen
// (a loop: the walk would never end), is `bad`.
static bool walk(const CPU& cpu, std::uint16_t& bad, bool print) {
    bool seen[HEAP_HI + 1 - HEAP_LO] = {}; // one per heap address, all false
    for (std::uint16_t node = cpu.free_head; node != 0; node = node_next(cpu, node)) {
        if (!node_ok(cpu, node) || seen[node - HEAP_LO]) {
            bad = node;
            return false;
        }
        seen[node - HEAP_LO] = true;
        if (print)
            std::cout << hex(node, 4) << " (" << static_cast<int>(node_size(cpu, node))
                      << " bytes) -> ";
    }
    return true;
}

bool heap_check(const CPU& cpu, std::uint16_t& bad) { return walk(cpu, bad, false); }

// Take `node` off the list; `prev` is the node before it, 0 for the head.
static void take_out(CPU& cpu, std::uint16_t prev, std::uint16_t node) {
    if (prev == 0)
        cpu.free_head = node_next(cpu, node);
    else
        set_next(cpu, prev, node_next(cpu, node));
}

HeapResult heap_alloc(CPU& cpu, Byte n, std::uint16_t& at, bool& reused) {
    reused = false;
    if (n == 0) {
        // The ISA formula as it is: 0 bytes always fit, nothing moves.
        at = cpu.heap_ptr;
        return HeapResult::ok;
    }
    std::uint16_t bad = 0;
    if (!heap_check(cpu, bad))
        return HeapResult::broken_list; // before anything changes

    // First fit. A bigger node gives away its tail: the node stays where it
    // is with a smaller size, so no link has to change.
    for (std::uint16_t prev = 0, node = cpu.free_head; node != 0;
         prev = node, node = node_next(cpu, node)) {
        const Byte size = node_size(cpu, node);
        if (size == n) {
            take_out(cpu, prev, node);
            at = node;
            reused = true;
            return HeapResult::ok;
        }
        if (size >= n + NODE_SIZE) {
            set_size(cpu, node, static_cast<Byte>(size - n));
            at = static_cast<std::uint16_t>(node + size - n);
            reused = true;
            return HeapResult::ok;
        }
    }

    // Bump, as in the ISA. The block may end on HEAP_HI itself; counted in
    // size_t, so heap_ptr + n is never cut to 16 bits.
    if (static_cast<std::size_t>(cpu.heap_ptr) + n > HEAP_HI + 1)
        return HeapResult::no_room;
    at = cpu.heap_ptr;
    cpu.heap_ptr = static_cast<std::uint16_t>(cpu.heap_ptr + n);
    return HeapResult::ok;
}

HeapResult heap_free(CPU& cpu, std::uint16_t at, Byte n, bool& top) {
    top = false;
    // The list first, as in ALLOC: a broken one is reported whatever the
    // block is, so the same FREE never stops one run and not another.
    std::uint16_t bad = 0;
    if (!heap_check(cpu, bad))
        return HeapResult::broken_list;
    if (n == 0 || at < HEAP_LO || at + n > cpu.heap_ptr)
        return HeapResult::bad_block;
    // A block that overlaps a free one was freed already (or never was a block).
    for (std::uint16_t node = cpu.free_head; node != 0; node = node_next(cpu, node)) {
        if (node < at + n && at < node + node_size(cpu, node))
            return HeapResult::already_free;
    }

    if (at + n == cpu.heap_ptr) {
        // The top block: heap_ptr comes down to it. Free blocks that now end
        // at heap_ptr are not between used ones any more: down past them too.
        top = true;
        cpu.heap_ptr = at;
        bool lowered = true;
        while (lowered) {
            lowered = false;
            for (std::uint16_t prev = 0, node = cpu.free_head; node != 0;
                 prev = node, node = node_next(cpu, node)) {
                if (node + node_size(cpu, node) == cpu.heap_ptr) {
                    take_out(cpu, prev, node);
                    cpu.heap_ptr = node;
                    lowered = true;
                    break;
                }
            }
        }
        return HeapResult::ok;
    }

    if (n < NODE_SIZE)
        return HeapResult::too_small;
    // Any other block: a node at its start, pushed on the head of the list.
    // The rest of its bytes stay as they were.
    set_next(cpu, at, cpu.free_head);
    set_size(cpu, at, n);
    cpu.free_head = at;
    return HeapResult::ok;
}

void heap_print(const CPU& cpu) {
    std::cout << "heap " << hex(HEAP_LO, 4) << '-' << hex(HEAP_HI, 4)
              << ": heap_ptr=" << hex(cpu.heap_ptr, 4) << ", " << cpu.heap_ptr - HEAP_LO
              << " used, " << HEAP_HI + 1 - cpu.heap_ptr << " left\n";
    std::cout << "free list: ";
    std::uint16_t bad = 0;
    if (cpu.free_head == 0)
        std::cout << "empty\n";
    else if (walk(cpu, bad, true))
        std::cout << "end\n";
    else
        std::cout << "broken at " << hex(bad, 4) << '\n';
}
