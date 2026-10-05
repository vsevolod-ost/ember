// stack.hpp — the guest stack (Lab 7) as an abstract data type.
//
// ISA §5/§6: the stack is the bytes STACK_LO..STACK_HI of guest memory and
// SP, "empty descending": SP is the next free byte and goes down. PUSH is
// mem[SP] = v; SP -= 1, POP is SP += 1; v = mem[SP]. This header is all the
// rest of ember (cpu.cpp, main.cpp) knows about it: the functions below. The
// ISA rules (bounds, byte order of 16-bit values) are in stack.cpp; how the
// bytes are kept is in stack_array.cpp OR stack_list.cpp, picked by CMake.
//
// Unlike the theory's `Stack { uint16_t data[64]; int top; }` (a host stack
// of its own), the values here are Bytes and the storage is the guest's
// memory: DEFENSE T1/T2 read mem[0xFFF] and mem[0xFFE] after a PUSH/CALL.
#pragma once

#include <cstddef>
#include <cstdint>

#include "memory.hpp"

struct CPU;
struct StackNode; // opaque: defined only in stack_list.cpp

// 256 bytes: SP is STACK_HI when empty and STACK_LO - 1 (0xEFF) when full.
// Only stack*.cpp look inside (layout.cpp only asks offsetof).
struct Stack {
    std::uint16_t sp = static_cast<std::uint16_t>(STACK_HI);  // the next free byte
    std::uint16_t low = static_cast<std::uint16_t>(STACK_HI); // the deepest SP so far
    StackNode* top = nullptr; // the newest node (list build); always nullptr in the array build
};

// Every refusal is false with nothing changed: not a byte, not SP.

// PUSH: put `v` at SP, SP -= 1. False: full (the byte would go below STACK_LO).
bool push(CPU& cpu, Byte v);
// POP: SP += 1, `out` = that byte. False: empty (SP is STACK_HI).
bool pop(CPU& cpu, Byte& out);
// CALL/PUSH H: two bytes, low first, then high (ISA §6). False: fewer than 2 free.
bool push16(CPU& cpu, std::uint16_t v);
// RET/POP H: high byte first, then low. False: fewer than 2 bytes on the stack.
bool pop16(CPU& cpu, std::uint16_t& out);
// What pop16 would give, without taking it (RET checks its target first).
bool peek16(const CPU& cpu, std::uint16_t& out);
// The i-th byte from the top (0 = the newest). False: fewer than i + 1 bytes.
bool stack_peek(const CPU& cpu, std::size_t i, Byte& out);

bool stack_empty(const Stack& s);
bool stack_full(const Stack& s);
std::size_t stack_used(const Stack& s); // bytes on the stack, 0..256
std::size_t stack_free(const Stack& s); // bytes a push can still take, 0..256
std::uint16_t stack_sp(const Stack& s);
std::uint16_t stack_deepest(const Stack& s); // the lowest SP since start or `reg sp`

// `reg sp`: SP = sp (STACK_LO - 1 .. STACK_HI); the bytes above it are the
// stack from now on. The deepest mark starts again here. False: out of range.
bool stack_set_sp(CPU& cpu, std::uint16_t sp);

// `stack [n]`: SP, used/free, the deepest SP, then the n newest bytes.
void stack_print(const CPU& cpu, std::size_t n);

// Empty the stack and give back any host memory it holds (the nodes of the
// list build). main calls it once, before the CPU goes away.
void stack_clear(Stack& s);
