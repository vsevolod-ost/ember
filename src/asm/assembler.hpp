// assembler.hpp — tokens to bytes (Lab 8, M2).
//
// Pass 0 expands .define (macro.hpp). Pass 1 walks the lines and counts:
// every label gets the address the next byte will have, every instruction
// adds its size from the ISA table (isa.hpp). Pass 2 walks the same lines
// again and writes the bytes, now that every label has an address. With
// AsmMode::one_pass there is only one walk: an operand that names a label
// not seen yet gets 00 bytes and goes into a queue, which is emptied after
// the last line (the "queue of deferred operands").
//
// The bytes go into a Program, not into Memory: a program with an error in
// its last line must not have loaded its first ones.
#pragma once

#include <cstdint>
#include <string>

#include "expr.hpp"
#include "label.hpp"
#include "memory.hpp"

enum class AsmMode { two_pass, one_pass };

// What an operand's bytes hold, for the range check: imm8 and .byte are one
// byte, the rest two (low byte first).
enum class Slot { imm8, imm16, addr16, byte, word };

// One line of the listing: the source line `line` put `count` bytes at `at`
// (count 0: a line with a label only, `at` is its address).
struct Placed {
    int line;
    std::uint16_t at;
    std::uint16_t count;
    Placed* next;
};

// One deferred operand of the single pass: the bytes at `at` wait for `expr`.
// Once patched, `expr` is gone and `text`/`value` say what was written.
struct Fixup {
    std::uint16_t at;
    Slot slot;
    Expr* expr;
    int line;
    std::string text;
    long value;
    Fixup* next;
};

struct Program {
    Byte bytes[MEM_SIZE]{};
    int owner[MEM_SIZE]{}; // the source line that wrote the byte; 0 = not written
    Label* labels = nullptr;
    Placed* placed = nullptr;
    Placed* placed_tail = nullptr;
    Fixup* patched = nullptr; // the single pass's queue, after it was emptied
    Fixup* patched_tail = nullptr;
};

// Assemble `src` into `p` (which must be fresh). False: `error` is
// "line N: ...", the first error found, and `p` holds nothing.
bool assemble(const std::string& src, AsmMode mode, Program& p, std::string& error);

// What `asm` prints: the listing (address, bytes, source line), the labels,
// the patched operands (single pass) and "assembled N bytes: ...".
void print_listing(const Program& p, const std::string& src);

// One listing line, laid out like the trace: "0000  28 00 08  LOADH H, LO"
// (the address, `n` of `bytes` in three columns, two spaces, the text), no
// spaces at the end. The text starts at column 17 whatever n is.
std::string listing_line(std::uint16_t at, const Byte* bytes, std::uint16_t n,
                         const std::string& text);

// Copy the written bytes into memory as the host (mem_set: the output port
// does not print). The other bytes of `mem` are left as they are.
void load_program(const Program& p, Memory& mem);

// Free the lists of `p`. `p.labels` may have been taken (set to nullptr) first.
void program_free(Program& p);
