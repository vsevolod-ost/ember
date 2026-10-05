// stack.cpp — the ISA's rules for the stack (ISA §5, §6), the same for both
// representations: where SP may go, which way it moves, the byte order of a
// 16-bit value, the deepest mark. Where the bytes are kept is not known
// here: stack_store.hpp's hooks keep them.
#include "stack.hpp"

#include <iostream>

#include "cpu.hpp"
#include "stack_store.hpp"

// SP of a full stack: the last byte, STACK_LO, is taken.
static const std::uint16_t SP_FULL = static_cast<std::uint16_t>(STACK_LO - 1);

bool stack_empty(const Stack& s) { return s.sp >= STACK_HI; }

bool stack_full(const Stack& s) { return s.sp < STACK_LO; }

std::size_t stack_used(const Stack& s) { return STACK_HI - s.sp; }

std::size_t stack_free(const Stack& s) { return s.sp - SP_FULL; }

std::uint16_t stack_sp(const Stack& s) { return s.sp; }

std::uint16_t stack_deepest(const Stack& s) { return s.low; }

bool push(CPU& cpu, Byte v) {
    // Overflow: the byte would be written below STACK_LO. Never wrap.
    if (stack_full(cpu.stack) || !store_push(cpu, v))
        return false;
    cpu.stack.sp = static_cast<std::uint16_t>(cpu.stack.sp - 1);
    if (cpu.stack.sp < cpu.stack.low)
        cpu.stack.low = cpu.stack.sp;
    return true;
}

bool pop(CPU& cpu, Byte& out) {
    // Underflow: SP is STACK_HI, the byte would be read above it.
    if (stack_empty(cpu.stack))
        return false;
    out = store_pop(cpu);
    cpu.stack.sp = static_cast<std::uint16_t>(cpu.stack.sp + 1);
    return true;
}

// Both bytes or none: the room for two is checked before the first push, so
// a CALL never leaves half an address behind.
bool push16(CPU& cpu, std::uint16_t v) {
    if (stack_free(cpu.stack) < 2)
        return false;
    if (!push(cpu, static_cast<Byte>(v & 0xFF))) // low byte first
        return false;
    if (!push(cpu, static_cast<Byte>(v >> 8))) {
        // Only a list without a node for the second byte gets here: take the
        // first one back. (A leftover mirror byte lies below SP: free space.)
        Byte undo = 0;
        pop(cpu, undo);
        return false;
    }
    return true;
}

bool peek16(const CPU& cpu, std::uint16_t& out) {
    if (stack_used(cpu.stack) < 2)
        return false;
    // The newest byte (the high one, pushed last) is popped first.
    out = static_cast<std::uint16_t>((store_peek(cpu, 0) << 8) | store_peek(cpu, 1));
    return true;
}

bool pop16(CPU& cpu, std::uint16_t& out) {
    if (stack_used(cpu.stack) < 2)
        return false;
    Byte hi = 0, lo = 0;
    pop(cpu, hi); // high byte first
    pop(cpu, lo);
    out = static_cast<std::uint16_t>((hi << 8) | lo);
    return true;
}

bool stack_peek(const CPU& cpu, std::size_t i, Byte& out) {
    if (i >= stack_used(cpu.stack))
        return false;
    out = store_peek(cpu, i);
    return true;
}

bool stack_set_sp(CPU& cpu, std::uint16_t sp) {
    if (sp < SP_FULL || sp > STACK_HI || !store_resync(cpu, sp))
        return false;
    cpu.stack.sp = sp;
    cpu.stack.low = sp;
    return true;
}

// "1 byte", "2 bytes".
static void print_bytes(std::size_t n) { std::cout << n << (n == 1 ? " byte" : " bytes"); }

void stack_print(const CPU& cpu, std::size_t n) {
    const Stack& s = cpu.stack;
    const std::size_t used = stack_used(s);
    std::cout << "SP=" << hex(s.sp, 4) << "  ";
    if (used == 0) {
        std::cout << "empty";
    } else {
        print_bytes(used);
        std::cout << " on the stack";
    }
    std::cout << ", " << stack_free(s) << " free (stack " << hex(STACK_LO, 4) << '-'
              << hex(STACK_HI, 4) << "); deepest SP=" << hex(s.low, 4) << " (";
    print_bytes(STACK_HI - s.low);
    std::cout << ")\n";
    // Newest first: the byte at SP + 1 is the one the next POP takes.
    for (std::size_t i = 0; i < used && i < n; ++i)
        std::cout << "  " << hex(static_cast<unsigned>(s.sp + 1 + i), 4) << "  "
                  << hex(store_peek(cpu, i), 2).substr(2) << (i == 0 ? "  <- top" : "") << '\n';
    if (used > n) {
        std::cout << "  ... ";
        print_bytes(used - n);
        std::cout << " more, up to " << hex(STACK_HI, 4) << '\n';
    }
}

void stack_clear(Stack& s) {
    store_clear(s);
    s.sp = static_cast<std::uint16_t>(STACK_HI);
}
