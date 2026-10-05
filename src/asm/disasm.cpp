// disasm.cpp — walk the bytes with the sizes of the ISA table, print each
// instruction with the table's text.
#include "disasm.hpp"

#include <iostream>

#include "assembler.hpp"
#include "cpu.hpp"
#include "isa.hpp"

// The size of what starts at `at` when the range ends at `hi`: the row's
// size, or 1 for a byte printed as .byte (no row, or the row does not fit).
static std::uint16_t piece_size(const Memory& mem, std::size_t at, std::size_t hi) {
    const IsaRow* row = isa_row(mem_get(mem, at));
    return row && at + row_size(*row) <= hi ? row_size(*row) : 1;
}

void disasm_ranges(const Memory& mem, const std::uint16_t lo[], const std::uint16_t hi[],
                   std::size_t n, const Label* labels) {
    // First where every printed piece starts: only a label printed in front
    // of one can be used as an operand name, or the text would not assemble.
    bool starts[MEM_SIZE]{};
    for (std::size_t r = 0; r < n; ++r)
        for (std::size_t at = lo[r]; at < hi[r]; at += piece_size(mem, at, hi[r]))
            starts[at] = true;

    std::size_t expected = 0; // where the text so far would put the next byte
    for (std::size_t r = 0; r < n; ++r) {
        if (lo[r] == hi[r])
            continue; // empty: no bytes, so no .org either
        if (lo[r] != expected)
            std::cout << std::string(16, ' ') << ".org " << hex(lo[r], 4) << '\n';
        std::size_t at = lo[r];
        while (at < hi[r]) {
            const std::uint16_t size = piece_size(mem, at, hi[r]);
            Byte bytes[3] = {mem_get(mem, at), 0, 0};
            for (std::uint16_t i = 1; i < size; ++i)
                bytes[i] = mem_get(mem, at + i);

            std::string text;
            for (const Label* l = labels; l; l = l->next)
                if (l->addr == at)
                    text += l->name + ": ";
            const IsaRow* row = isa_row(bytes[0]);
            const auto w16 = static_cast<std::uint16_t>(bytes[1] | (bytes[2] << 8));
            const Operand o = row ? row_operand(*row) : Operand::none;
            if (!row || size != row_size(*row)) {
                text += ".byte " + hex(bytes[0], 2);
            } else if (o == Operand::addr16 && w16 >= MEM_SIZE) {
                // The assembler refuses an address outside the box, so the
                // bytes go as data; what they would mean is the comment.
                text += ".byte " + hex(bytes[0], 2) + ", " + hex(bytes[1], 2) + ", " +
                        hex(bytes[2], 2) + "  ; " + row_text(*row, bytes[1], w16);
            } else {
                const Label* name =
                    o == Operand::addr16 || o == Operand::imm16 ? label_at(labels, w16) : nullptr;
                if (name && w16 < MEM_SIZE && starts[w16])
                    text += row_fill(*row, name->name);
                else
                    text += row_text(*row, bytes[1], w16);
            }
            std::cout << listing_line(static_cast<std::uint16_t>(at), bytes, size, text) << '\n';
            at += size;
        }
        expected = hi[r];
    }
}
