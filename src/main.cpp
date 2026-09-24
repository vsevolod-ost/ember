// main.cpp — the prompt.
//
// GIVEN, in full. This file is scaffolding: it reads a line, splits it into
// words, and calls one of your functions. It uses `while`, `if` and functions,
// which the course only explains properly in Labs 4 and 7. That is on purpose —
// in Lab 1 you read this file, you do not write it.
//
// What you add in later labs is one more `else if` branch per command.
#include <iostream>
#include <sstream>
#include <string>

#include "alu.hpp"
#include "cpu.hpp"
#include "dump.hpp"
#include "memory.hpp"

// Turn a word into a number. Accepts decimal (65) and hex (0x41).
// Returns false if the word is not a number at all.
static bool parse_number(const std::string& word, long& out) {
    try {
        std::size_t used = 0;
        // base 0 means: look at the prefix. "0x41" is hex, "65" is decimal.
        out = std::stol(word, &used, 0);
        return used == word.size(); // reject things like "12abc"
    } catch (...) {
        return false;
    }
}

// Run one ALU op by name on fresh (all-zero) flags. False if the name is unknown.
static bool alu_by_name(const std::string& name, Byte a, Byte b, Byte& r, Flags& f) {
    if (name == "add")
        r = alu_add(a, b, f);
    else if (name == "sub")
        r = alu_sub(a, b, f);
    else if (name == "and")
        r = alu_and(a, b, f);
    else if (name == "or")
        r = alu_or(a, b, f);
    else if (name == "xor")
        r = alu_xor(a, b, f);
    else if (name == "not")
        r = alu_not(a, f);
    else if (name == "shl")
        r = alu_shl(a, f);
    else if (name == "shr")
        r = alu_shr(a, f);
    else if (name == "inc")
        r = alu_inc(a, f);
    else if (name == "dec")
        r = alu_dec(a, f);
    else
        return false;
    return true;
}

static void report(StepResult r, const CPU& cpu) {
    switch (r) {
    case StepResult::ok:
        break;
    case StepResult::halted:
        std::cout << "halted: step does nothing after HALT\n";
        break;
    case StepResult::bad_opcode:
        std::cout << "unknown opcode 0x" << std::hex << static_cast<int>(mem_get(*cpu.mem, cpu.pc))
                  << " at 0x" << cpu.pc << std::dec << "; nothing done\n";
        break;
    case StepResult::out_of_memory:
        std::cout << "instruction at 0x" << std::hex << cpu.pc << std::dec << " does not fit in 0.."
                  << MEM_SIZE - 1 << "; nothing done\n";
        break;
    }
}

static void print_help() {
    std::cout << "commands:\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  get <addr>        show one byte four ways\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  set16 <addr> <v>  write a 16-bit value, little-endian\n"
              << "  inc <addr>        add one to a byte (wraps 255 -> 0)\n"
              << "  alu <op> <a> [b]  add sub and or xor (a b); not shl shr inc dec (a)\n"
              << "  regs              print PC A B Z N C\n"
              << "  step              execute one instruction at PC\n"
              << "  run               step until HALT or an error\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

int main() {
    Memory mem; // 4096 bytes, on the stack, zeroed by the {} in memory.hpp
    CPU cpu{&mem};

    std::cout << "ember 0.1 - 4096 bytes of memory you can see. Type `help`.\n";

    std::string line;
    while (true) {
        std::cout << "ember> ";

        // getline reads one whole line. It returns false at end of input
        // (Ctrl-D), which is how the loop ends if you never type `quit`.
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        // Split the line into words: "set 0 65" -> cmd="set", args "0" and "65".
        std::istringstream words(line);
        std::string cmd;
        words >> cmd;

        if (cmd.empty()) {
            continue; // the user just pressed Enter
        } else if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "help") {
            print_help();
        } else if (cmd == "dump") {
            dump(mem);
        } else if (cmd == "get") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: get <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                show_byte(mem_get(mem, static_cast<std::size_t>(addr)));
            }
        } else if (cmd == "set") {
            std::string a, v;
            long addr = 0, value = 0;
            if (!(words >> a) || !(words >> v) || !parse_number(a, addr) ||
                !parse_number(v, value)) {
                std::cout << "usage: set <addr> <value>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (value < 0 || value > 255) {
                // A cell holds ONE byte. 256 does not fit. Lab 1, theory 3.
                std::cout << "a byte is 0..255, got " << value << '\n';
            } else if (!mem_set(mem, static_cast<std::size_t>(addr), static_cast<Byte>(value))) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1 << '\n';
            }
        } else if (cmd == "set16") {
            std::string a, v;
            long addr = 0, value = 0;
            if (!(words >> a) || !(words >> v) || !parse_number(a, addr) ||
                !parse_number(v, value)) {
                std::cout << "usage: set16 <addr> <value>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (value < 0 || value > 0xFFFF) {
                // Two cells hold TWO bytes: 0..65535.
                std::cout << "a 16-bit value is 0..65535, got " << value << '\n';
            } else if (static_cast<std::size_t>(addr) + 1 >= MEM_SIZE) {
                // Both cells must fit: the last valid start address is MEM_SIZE - 2.
                std::cout << "set16 needs two bytes at " << addr << " and " << addr + 1
                          << "; the box is 0.." << MEM_SIZE - 1 << '\n';
            } else {
                // Little-endian: the low byte goes in first, at the lower address.
                mem_set(mem, static_cast<std::size_t>(addr), static_cast<Byte>(value & 0xFF));
                mem_set(mem, static_cast<std::size_t>(addr) + 1,
                        static_cast<Byte>((value >> 8) & 0xFF));
            }
        } else if (cmd == "inc") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: inc <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                // Read, add one, write back. `b + 1` is computed as int (256 for
                // b == 255); the cast back to Byte keeps the low 8 bits -> 0.
                Byte b = mem_get(mem, static_cast<std::size_t>(addr));
                if (!mem_set(mem, static_cast<std::size_t>(addr), static_cast<Byte>(b + 1))) {
                    std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1 << '\n';
                }
            }
        } else if (cmd == "alu") {
            std::string name, x, y;
            long a = 0, b = 0;
            words >> name >> x;
            const bool one =
                name == "not" || name == "shl" || name == "shr" || name == "inc" || name == "dec";
            const bool two =
                name == "add" || name == "sub" || name == "and" || name == "or" || name == "xor";
            Byte r = 0;
            Flags f{};
            if (!name.empty() && !one && !two) {
                std::cout << "unknown alu op: " << name << '\n';
            } else if (!parse_number(x, a) || (!one && (!(words >> y) || !parse_number(y, b)))) {
                std::cout << "usage: alu <op> <a> [b]\n";
            } else if (a < 0 || a > 255 || b < 0 || b > 255) {
                std::cout << "a byte is 0..255\n";
            } else {
                alu_by_name(name, static_cast<Byte>(a), static_cast<Byte>(b), r, f);
                std::cout << "result=" << static_cast<int>(r) << "  Z=" << f.z << " N=" << f.n
                          << " C=" << f.c << '\n';
            }
        } else if (cmd == "regs") {
            dump_regs(cpu);
        } else if (cmd == "step") {
            report(step(cpu), cpu);
        } else if (cmd == "run") {
            StepResult r = step(cpu);
            while (r == StepResult::ok && !cpu.halted)
                r = step(cpu);
            report(r, cpu);
        } else {
            std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    return 0; // 0 means "success" to the operating system
}
