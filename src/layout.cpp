// layout.cpp — the compiler decides where fields go; offsetof and sizeof are
// how a program asks it. Padding is never named in the source: it is what
// is left between one field's end and the next one's offset, and after the
// last field up to sizeof. (offsetof is allowed here: these structs are
// standard-layout — plain public fields, no virtual functions.)
#include "layout.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <iterator>

#include "cpu.hpp"
#include "sprite.hpp"
#include "value.hpp"

struct Field {
    const char* name;
    std::size_t off, size;
};

static void print_row(std::size_t off, std::size_t size, const char* name) {
    std::cout << std::setw(5) << off << std::setw(5) << size << ' ' << name << '\n';
}

// Fields in declaration order, so offsets only grow.
static void print_struct(const char* name, std::size_t size, std::size_t align, const Field* fields,
                         std::size_t count) {
    std::cout << name << ": sizeof=" << size << " alignof=" << align << '\n'
              << "  off size field\n";
    std::size_t end = 0, used = 0, padding = 0;
    for (std::size_t i = 0; i < count; ++i) {
        if (fields[i].off > end) {
            print_row(end, fields[i].off - end, "(padding)");
            padding += fields[i].off - end;
        }
        print_row(fields[i].off, fields[i].size, fields[i].name);
        used += fields[i].size;
        end = fields[i].off + fields[i].size;
    }
    if (size > end) {
        print_row(end, size - end, "(padding)");
        padding += size - end;
    }
    std::cout << "  fields " << used << " + padding " << padding << " = " << size << '\n';
}

// Each struct: its fields in declaration order, written out by hand.
// sizeof(CPU::pc) is the size of the member without a CPU object.
bool print_layout(const std::string& what) {
    const bool all = what.empty();
    if (!all && what != "cpu" && what != "flags" && what != "instruction" && what != "sprite" &&
        what != "value")
        return false;
    if (all || what == "cpu") {
        const Field f[] = {
            {"mem", offsetof(CPU, mem), sizeof(CPU::mem)},
            {"pc", offsetof(CPU, pc), sizeof(CPU::pc)},
            {"h", offsetof(CPU, h), sizeof(CPU::h)},
            {"a", offsetof(CPU, a), sizeof(CPU::a)},
            {"b", offsetof(CPU, b), sizeof(CPU::b)},
            {"f (Flags)", offsetof(CPU, f), sizeof(CPU::f)},
            {"halted", offsetof(CPU, halted), sizeof(CPU::halted)},
            {"ticks", offsetof(CPU, ticks), sizeof(CPU::ticks)},
            {"heap_ptr", offsetof(CPU, heap_ptr), sizeof(CPU::heap_ptr)},
            {"free_head", offsetof(CPU, free_head), sizeof(CPU::free_head)},
        };
        print_struct("CPU", sizeof(CPU), alignof(CPU), f, std::size(f));
    }
    if (all || what == "flags") {
        const Field f[] = {
            {"z", offsetof(Flags, z), sizeof(Flags::z)},
            {"n", offsetof(Flags, n), sizeof(Flags::n)},
            {"c", offsetof(Flags, c), sizeof(Flags::c)},
        };
        print_struct("Flags", sizeof(Flags), alignof(Flags), f, std::size(f));
    }
    if (all || what == "instruction") {
        const Field f[] = {
            {"at", offsetof(Instruction, at), sizeof(Instruction::at)},
            {"op (Op)", offsetof(Instruction, op), sizeof(Instruction::op)},
            {"size", offsetof(Instruction, size), sizeof(Instruction::size)},
            {"bytes[3]", offsetof(Instruction, bytes), sizeof(Instruction::bytes)},
            {"w16", offsetof(Instruction, w16), sizeof(Instruction::w16)},
        };
        print_struct("Instruction", sizeof(Instruction), alignof(Instruction), f, std::size(f));
    }
    if (all || what == "sprite") {
        const Field f[] = {
            {"x", offsetof(Sprite, x), sizeof(Sprite::x)},
            {"y", offsetof(Sprite, y), sizeof(Sprite::y)},
            {"vx", offsetof(Sprite, vx), sizeof(Sprite::vx)},
            {"vy", offsetof(Sprite, vy), sizeof(Sprite::vy)},
            {"alive", offsetof(Sprite, alive), sizeof(Sprite::alive)},
        };
        print_struct("Sprite", sizeof(Sprite), alignof(Sprite), f, std::size(f));
    }
    if (all || what == "value") {
        const Field f[] = {
            {"kind (Kind)", offsetof(Value, kind), sizeof(Value::kind)},
            {"bits", offsetof(Value, bits), sizeof(Value::bits)},
        };
        print_struct("Value", sizeof(Value), alignof(Value), f, std::size(f));
    }
    return true;
}
