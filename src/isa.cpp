// isa.cpp — the rows of ISA.uk.md §5, and the few questions asked of them.
#include "isa.hpp"

#include <cstring>

#include "cpu.hpp"

// The text column is copied from the ISA, letter for letter: the assembler
// lexes it to know the shape of the line (assembler.cpp), so a typo here is
// a typo in the language. *opt* rows are here too; the last three are ours.
static const IsaRow ROWS[] = {
    // 0x0_ — control and output
    {Op::Halt, "HALT"},
    {Op::Nop, "NOP"},
    {Op::Out, "OUT"},
    {Op::Outn, "OUTN"},
    {Op::Outs, "OUTS addr16"},
    // 0x1_ — ALU
    {Op::Add, "ADD A, B"},
    {Op::Sub, "SUB A, B"},
    {Op::And, "AND A, B"},
    {Op::Or, "OR A, B"},
    {Op::Xor, "XOR A, B"},
    {Op::Not, "NOT A"},
    {Op::Shl, "SHL A"},
    {Op::Shr, "SHR A"},
    {Op::Inc, "INC A"},
    {Op::Dec, "DEC A"},
    {Op::Cmp, "CMP A, B"},
    {Op::Mul, "MUL A, B"},
    // 0x2_ — data movement
    {Op::LoadiA, "LOADI A, imm8"},
    {Op::LoadiB, "LOADI B, imm8"},
    {Op::LoadA, "LOAD A, [addr16]"},
    {Op::LoadB, "LOAD B, [addr16]"},
    {Op::StoreA, "STORE [addr16], A"},
    {Op::StoreB, "STORE [addr16], B"},
    {Op::MovAB, "MOV A, B"},
    {Op::MovBA, "MOV B, A"},
    {Op::LoadH, "LOADH H, imm16"},
    {Op::LoadAH, "LOAD A, [H]"},
    {Op::StoreHA, "STORE [H], A"},
    {Op::IncH, "INCH"},
    {Op::DecH, "DECH"},
    {Op::HLow, "HLOW"},
    // 0x3_ — jumps
    {Op::Jmp, "JMP addr16"},
    {Op::Jz, "JZ addr16"},
    {Op::Jnz, "JNZ addr16"},
    {Op::Jc, "JC addr16"},
    {Op::Jnc, "JNC addr16"},
    // 0x4_ — screen
    {Op::Cls, "CLS"},
    {Op::Plot, "PLOT"},
    {Op::Unplot, "UNPLOT"},
    {Op::Show, "SHOW"},
    // 0x5_ — stack and calls
    {Op::PushA, "PUSH A"},
    {Op::PopA, "POP A"},
    {Op::PushB, "PUSH B"},
    {Op::PopB, "POP B"},
    {Op::Call, "CALL addr16"},
    {Op::Ret, "RET"},
    {Op::PushH, "PUSH H"},
    {Op::PopH, "POP H"},
    // 0x6_ — system
    {Op::Alloc, "ALLOC"},
    // 0x7_ — own extensions (Lab 4: JG/JL, Lab 6: FREE)
    {Op::Jg, "JG addr16"},
    {Op::Jl, "JL addr16"},
    {Op::Free, "FREE"},
};

static const std::size_t ROW_COUNT = sizeof(ROWS) / sizeof(ROWS[0]);

// The placeholder words and what they stand for.
static const char* const IMM8 = "imm8";
static const char* const IMM16 = "imm16";
static const char* const ADDR16 = "addr16";

const IsaRow* isa_row(Byte op) {
    // step() asks this for every instruction, so the rows are indexed by
    // their byte once, the first time anyone asks.
    struct Index {
        const IsaRow* by_byte[256];
    };
    static const Index index = [] {
        Index ix{};
        for (std::size_t i = 0; i < ROW_COUNT; ++i)
            ix.by_byte[static_cast<Byte>(ROWS[i].op)] = &ROWS[i];
        return ix;
    }();
    return index.by_byte[op];
}

const IsaRow* isa_rows(std::size_t& count) {
    count = ROW_COUNT;
    return ROWS;
}

// Where the placeholder starts in the row's text, and how long it is; npos
// for a row without one.
static std::size_t placeholder(const IsaRow& row, std::size_t& length) {
    for (const char* word : {IMM8, IMM16, ADDR16}) {
        if (const char* at = std::strstr(row.text, word)) {
            length = std::strlen(word);
            return static_cast<std::size_t>(at - row.text);
        }
    }
    length = 0;
    return std::string::npos;
}

Operand row_operand(const IsaRow& row) {
    if (std::strstr(row.text, IMM8))
        return Operand::imm8;
    if (std::strstr(row.text, IMM16))
        return Operand::imm16;
    if (std::strstr(row.text, ADDR16))
        return Operand::addr16;
    return Operand::none;
}

std::uint16_t row_size(const IsaRow& row) {
    switch (row_operand(row)) {
    case Operand::none:
        return 1;
    case Operand::imm8:
        return 2; // opcode, imm8
    case Operand::imm16:
    case Operand::addr16:
        return 3; // opcode, then a 16-bit operand low byte first
    }
    return 0;
}

std::string row_fill(const IsaRow& row, const std::string& operand) {
    std::string text = row.text;
    std::size_t length = 0;
    const std::size_t at = placeholder(row, length);
    if (at != std::string::npos)
        text.replace(at, length, operand);
    return text;
}

std::string row_text(const IsaRow& row, Byte imm8, std::uint16_t w16) {
    switch (row_operand(row)) {
    case Operand::none:
        return row.text;
    case Operand::imm8:
        return row_fill(row, hex(imm8, 2));
    case Operand::imm16:
    case Operand::addr16:
        return row_fill(row, hex(w16, 4));
    }
    return row.text;
}

bool is_mnemonic(const std::string& word) {
    for (std::size_t i = 0; i < ROW_COUNT; ++i) {
        const std::string text = ROWS[i].text;
        if (text.substr(0, text.find(' ')) == word)
            return true;
    }
    return false;
}
