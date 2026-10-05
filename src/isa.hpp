// isa.hpp — the instruction table of ISA.uk.md §5 in one place (Lab 8).
//
// One row per opcode: the byte and the "Мнемоніка" column as the ISA writes
// it, placeholder included ("LOAD A, [addr16]"). The placeholder is the only
// place the size comes from: none = 1 byte, imm8 = 2, imm16/addr16 = 3. So
// step() (through instr_size), the trace, the assembler and the disassembler
// read the same row and cannot disagree about a size (notes §4).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "memory.hpp"
#include "opcodes.hpp"

// What follows the opcode byte.
enum class Operand : std::uint8_t { none, imm8, imm16, addr16 };

struct IsaRow {
    Op op;
    const char* text; // the ISA's mnemonic column, word for word
};

// The row of opcode byte `op`; nullptr if the ISA (and our extensions) have
// none: that byte is an unknown opcode.
const IsaRow* isa_row(Byte op);

// All rows in ISA order, then the own extensions 0x70-0x72.
const IsaRow* isa_rows(std::size_t& count);

Operand row_operand(const IsaRow& row);

// 1 + the operand bytes: the "Розмір" column, computed from the placeholder.
std::uint16_t row_size(const IsaRow& row);

// The row's text with the placeholder replaced by `operand`:
// row_fill(LOAD A, [addr16], "data") == "LOAD A, [data]".
std::string row_fill(const IsaRow& row, const std::string& operand);

// The row's text with the operand as a number: imm8 as 0x and two hex
// digits, imm16/addr16 as 0x and four: "LOADI A, 0x05", "JMP 0x0123".
std::string row_text(const IsaRow& row, Byte imm8, std::uint16_t w16);

// Is `word` the first word of some row (HALT, LOADI, ...)? Case matters.
bool is_mnemonic(const std::string& word);
