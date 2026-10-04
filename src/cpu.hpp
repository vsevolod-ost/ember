// cpu.hpp — registers and one step of the processor.
#pragma once

#include <cstdint>
#include <string>

#include "alu.hpp"
#include "memory.hpp"

// Extension: a byte in the spare region 0xB00-0xBFF that mirrors the flags
// after every step, as 0b00000ZNC, so a guest program can read them.
const std::size_t STATUS_ADDR = 0xB00;

// One HOST pointer to the whole box, then GUEST addresses: pc and h are plain
// numbers 0..0xFFF that mean something inside ember (you can see them in
// `regs`, a program can add to them), not Byte* into mem->data.
struct CPU {
    Memory* mem;
    std::uint16_t pc = 0; // address of the NEXT instruction
    std::uint16_t h = 0;  // address register: where LOAD A, [H] reads
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

// Size in bytes from the ISA table (opcode + operands); 0 = not an opcode
// implemented so far.
std::uint16_t instr_size(Byte op);

enum class StepResult {
    ok,
    halted,        // HALT was executed earlier; nothing happened
    bad_opcode,    // not in the ISA rows implemented so far; nothing happened
    out_of_memory, // the instruction at PC does not fit below MEM_SIZE; nothing happened
    bad_address,   // a LOAD/STORE operand (addr16 or H) is >= MEM_SIZE; nothing happened
};

// Execute the instruction at PC. With `trace`, print one line for it.
StepResult step(CPU& cpu, bool trace);

// `v` as "0x" and `digits` hex digits: hex(0x800, 4) == "0x0800".
std::string hex(unsigned v, int digits);

// The instruction at `at` as text, operands filled in: "LOAD A, [0x0800]".
std::string disasm(const Memory& mem, std::uint16_t at);

// The guest address the LOAD/STORE at PC reads or writes (addr16 or H).
std::uint16_t data_address(const CPU& cpu);

// Print PC A B H Z N C on one line.
void dump_regs(const CPU& cpu);
