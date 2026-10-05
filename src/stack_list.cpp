// stack_list.cpp — the stack as linked nodes on the HOST heap, one per byte,
// the newest at Stack::top (EMBER_STACK=list). The same stack.hpp, stack.cpp
// and cpu.cpp as the array build: only this file is different.
//
// The ISA still says PUSH writes mem[SP], so a push also writes a mirror
// byte into guest memory (dump, get and LOAD see what the array build
// shows). But POP and RET take the value from the node, not from memory: a
// guest STORE into the live stack changes what they get only in the array
// build. Nodes come from new (std::nothrow): no exceptions, nullptr = no room.
#include <new>

#include "cpu.hpp"
#include "stack_store.hpp"

struct StackNode {
    Byte value;
    StackNode* next; // the node pushed before this one; nullptr at the bottom
};

// A node on top of `top`; nullptr if the host has no memory for it.
static StackNode* make_node(Byte v, StackNode* top) { return new (std::nothrow) StackNode{v, top}; }

static void free_nodes(StackNode* n) {
    while (n != nullptr) {
        StackNode* next = n->next;
        delete n;
        n = next;
    }
}

bool store_push(CPU& cpu, Byte v) {
    StackNode* n = make_node(v, cpu.stack.top);
    if (n == nullptr)
        return false;
    guest_write(cpu, cpu.stack.sp, v); // the mirror byte at mem[SP]
    cpu.stack.top = n;
    return true;
}

Byte store_pop(CPU& cpu) {
    StackNode* n = cpu.stack.top;
    if (n == nullptr) // stack.cpp never pops an empty stack
        return 0;
    const Byte v = n->value;
    cpu.stack.top = n->next;
    delete n;
    return v;
}

Byte store_peek(const CPU& cpu, std::size_t i) {
    const StackNode* n = cpu.stack.top;
    for (; n != nullptr && i > 0; --i)
        n = n->next;
    return n != nullptr ? n->value : 0;
}

// `reg sp`: the nodes are built again from the guest bytes sp + 1 .. STACK_HI,
// the oldest (at STACK_HI) first. A new list is built before the old one is
// freed, so running out of host memory changes nothing.
bool store_resync(CPU& cpu, std::uint16_t sp) {
    StackNode* top = nullptr;
    for (std::size_t addr = STACK_HI; addr > sp; --addr) {
        Byte v = 0;
        guest_read(cpu, addr, v);
        StackNode* n = make_node(v, top);
        if (n == nullptr) {
            free_nodes(top);
            return false;
        }
        top = n;
    }
    free_nodes(cpu.stack.top);
    cpu.stack.top = top;
    return true;
}

void store_clear(Stack& s) {
    free_nodes(s.top);
    s.top = nullptr;
}
