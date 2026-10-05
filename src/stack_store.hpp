// stack_store.hpp — PRIVATE to the stack: the hooks stack.cpp calls to keep
// the bytes. Two files define them, stack_array.cpp (the guest's own memory)
// and stack_list.cpp (host nodes); CMake links exactly one (EMBER_STACK).
// stack.cpp has checked the bounds before any of these is called, and moves
// SP itself afterwards.
#pragma once

#include <cstddef>
#include <cstdint>

#include "stack.hpp"

// Keep `v` as the new top, at guest address SP (SP not moved yet). False:
// no room to keep it (the list could not get a node); nothing changed.
bool store_push(CPU& cpu, Byte v);

// Take the top byte, the one at SP + 1 (SP not moved yet). Never empty.
Byte store_pop(CPU& cpu);

// The i-th byte from the top (0 = at SP + 1). i < stack_used.
Byte store_peek(const CPU& cpu, std::size_t i);

// SP is about to become `sp`: from now on the stack is the guest bytes
// sp + 1 .. STACK_HI. False: could not (nothing changed).
bool store_resync(CPU& cpu, std::uint16_t sp);

// Give back whatever the representation holds.
void store_clear(Stack& s);
