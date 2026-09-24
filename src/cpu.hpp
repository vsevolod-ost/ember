// cpu.hpp — registers and one step of the processor.
#pragma once

#include <cstdint>

#include "alu.hpp"
#include "memory.hpp"

// Extension: a byte in the spare region 0xB00-0xBFF that mirrors the flags
// after every step, as 0b00000ZNC, so a guest program can read them.
const std::size_t STATUS_ADDR = 0xB00;

struct CPU {
    Memory* mem;
    std::uint16_t pc = 0; // address of the NEXT instruction
    Byte a = 0, b = 0;
    Flags f{};
    bool halted = false; // set by HALT; every later step refuses
};

// An opcode byte is two fields: the high nibble is the group (0x1_ = ALU),
// the low nibble is the index inside the group.
struct Decoded {
    Byte group;
    Byte index;
};
Decoded decode(Byte op);

enum class StepResult {
    ok,
    halted,        // HALT was executed earlier; nothing happened
    bad_opcode,    // not in the ISA rows implemented so far; nothing happened
    out_of_memory, // the instruction at PC does not fit below MEM_SIZE; nothing happened
};

// Execute the instruction at PC and print one trace line for it.
StepResult step(CPU& cpu);

// Print PC A B Z N C on one line.
void dump_regs(const CPU& cpu);
