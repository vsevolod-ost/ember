// disasm.hpp — bytes back to text (Lab 8 Advanced), the assembler run
// backwards with the same ISA table.
#pragma once

#include <cstddef>
#include <cstdint>

#include "label.hpp"
#include "memory.hpp"

// Print the instructions in [lo[i], hi[i]) for each of the `n` ranges, one
// per line like the listing of `asm`: "0007  21 00     fib: LOADI B, 0x00".
// From column 17 on, the output is source that assembles back to the same
// bytes: a byte that is no opcode, or an instruction that would run past
// hi, is ".byte 0x.."; one whose addr16 is outside the box is its three
// bytes as .byte with the instruction in a comment; a range that does not
// start where the previous one ended starts with ".org"; an empty one
// prints nothing. The ranges must not overlap (main checks), or the same
// bytes and labels would come out twice. A label of
// `labels` (the last `asm`) at the start of a printed instruction is written
// before it, and an addr16/imm16 operand equal to such a label is written as
// its name.
void disasm_ranges(const Memory& mem, const std::uint16_t lo[], const std::uint16_t hi[],
                   std::size_t n, const Label* labels);
