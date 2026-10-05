// cpu.hpp — registers and one step of the processor.
#pragma once

#include <cstdint>
#include <string>

#include "alu.hpp"
#include "memory.hpp"
#include "opcodes.hpp"

// The spare region 0xB00-0xBFF (ISA §2). Two addresses do something when the
// GUEST touches them (*opt*, Lab 5); for the host they are plain memory.
const std::size_t PORT_OUT = 0xB00; // a guest write also prints the byte as a character
const std::size_t TICKS = 0xB01;    // a guest read gives the instructions run, mod 256

// Extension: a byte that mirrors the flags after every step, as 0b00000ZNC,
// so a guest program can read them. Lab 2 put it at 0xB00; it moved to 0xB02
// in Lab 5, when 0xB00 and 0xB01 became the port and the timer.
const std::size_t STATUS_ADDR = 0xB02;

// OUTS prints at most this many bytes, even if no 0 comes.
const std::size_t OUTS_MAX = 256;

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
    Byte ticks = 0;      // instructions executed, mod 256 (the guest reads it at TICKS)
    // The guest heap (Lab 6), not registers: `regs` does not show them.
    // heap_ptr needs an even address, so one padding byte sits before it.
    std::uint16_t heap_ptr = HEAP_LO; // the next fresh block starts here (ALLOC)
    std::uint16_t free_head = 0;      // first block of the free list (FREE); 0 = none
};

// What step read at PC before executing it: the ISA row and the bytes.
// `op` is an Op only once instr_size knew the byte.
struct Instruction {
    std::uint16_t at;  // its address: PC before the step
    Op op;             // bytes[0] as an Op
    Byte size;         // 1..3, the "розмір" column of the ISA
    Byte bytes[3];     // opcode, then operands; the unused ones are 0
    std::uint16_t w16; // bytes[1..2] as one value, low byte first (size 3 only)
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
    bad_address,   // a LOAD/STORE operand (addr16 or H), an OUTS operand or a taken
                   // jump's target is >= MEM_SIZE; nothing happened
    bad_heap,      // ALLOC/FREE found the free list broken (the guest wrote into a
                   // freed block); nothing happened
};

// Every byte the GUEST reads or writes by address goes through these two:
// false (and nothing done) for an address outside the box. They are where the
// output port (a write to PORT_OUT prints) and the timer (TICKS) live.
bool guest_read(const CPU& cpu, std::size_t addr, Byte& out);
bool guest_write(CPU& cpu, std::size_t addr, Byte value);

// Execute the instruction at PC. With `trace`, print one line for it.
StepResult step(CPU& cpu, bool trace);

// `v` as "0x" and `digits` hex digits: hex(0x800, 4) == "0x0800".
std::string hex(unsigned v, int digits);

// The instruction at `at` as text, operands filled in: "LOAD A, [0x0800]";
// "?" if there is none there (an unknown byte, or it runs past the box).
std::string disasm(const Memory& mem, std::uint16_t at);

// The guest address the instruction at PC uses: what a LOAD/STORE reads or
// writes (addr16 or H), where OUTS starts, where a jump goes (addr16).
std::uint16_t data_address(const CPU& cpu);

// Print PC A B H Z N C on one line.
void dump_regs(const CPU& cpu);
