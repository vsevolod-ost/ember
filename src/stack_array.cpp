// stack_array.cpp — the stack kept where the ISA puts it: the guest's own
// bytes STACK_LO..STACK_HI, an array indexed by SP. Built by default
// (EMBER_STACK=array). Every byte goes through guest_write/guest_read like
// any other guest access; there is nothing of the host's to allocate or free.
#include "cpu.hpp"
#include "stack_store.hpp"

bool store_push(CPU& cpu, Byte v) { return guest_write(cpu, cpu.stack.sp, v); }

Byte store_pop(CPU& cpu) { return store_peek(cpu, 0); }

Byte store_peek(const CPU& cpu, std::size_t i) {
    Byte v = 0;
    guest_read(cpu, cpu.stack.sp + 1 + i, v);
    return v;
}

// The bytes above the new SP are already where they belong.
bool store_resync(CPU&, std::uint16_t) { return true; }

void store_clear(Stack&) {}
