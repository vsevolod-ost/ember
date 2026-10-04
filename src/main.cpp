// main.cpp — the prompt.
//
// GIVEN, in full. This file is scaffolding: it reads a line, splits it into
// words, and calls one of your functions. It uses `while`, `if` and functions,
// which the course only explains properly in Labs 4 and 7. That is on purpose —
// in Lab 1 you read this file, you do not write it.
//
// What you add in later labs is one more `else if` branch per command.
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "alu.hpp"
#include "cpu.hpp"
#include "dump.hpp"
#include "guest_io.hpp"
#include "hexdump.hpp"
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

// The second word of the line ("" if none), to pick a branch before the
// given one takes the command: `reg pc ...`, `dump 0x800`.
static std::string second_word(const std::string& line) {
    std::istringstream words(line);
    std::string first, second;
    words >> first >> second;
    return second;
}

// True for a number >= MEM_SIZE: an address past the end of the box.
static bool past_the_box(const std::string& word) {
    long addr = 0;
    return parse_number(word, addr) && addr >= 0 && static_cast<std::size_t>(addr) >= MEM_SIZE;
}

static void report(StepResult r, const CPU& cpu) {
    guest_end_line(); // never glue a message to guest output
    switch (r) {
    case StepResult::ok:
        break;
    case StepResult::halted:
        std::cout << "halted: step does nothing after HALT\n";
        break;
    case StepResult::bad_opcode:
        std::cout << "unknown opcode " << hex(mem_get(*cpu.mem, cpu.pc), 2) << " at "
                  << hex(cpu.pc, 4) << "; nothing done\n";
        break;
    case StepResult::out_of_memory:
        std::cout << "instruction at " << hex(cpu.pc, 4) << " does not fit in 0.." << MEM_SIZE - 1
                  << "; nothing done\n";
        break;
    case StepResult::bad_address:
        std::cout << "address " << hex(data_address(cpu), 4) << " is outside 0.." << MEM_SIZE - 1
                  << " (" << disasm(*cpu.mem, cpu.pc) << " at " << hex(cpu.pc, 4)
                  << "); nothing done\n";
        break;
    }
}

static void print_help() {
    std::cout << "commands:\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  dump <addr> [n]   print the rows holding n bytes from addr (n=16)\n"
              << "  get <addr>        show one byte four ways\n"
              << "  get16 <addr>      read a 16-bit value, little-endian\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  set16 <addr> <v>  write a 16-bit value, little-endian\n"
              << "  reg <a|b> <v>     put 0..255 into register A or B\n"
              << "  reg <pc|h> <v>    put 0..65535 into PC or H\n"
              << "  inc <addr>        add one to a byte (wraps 255 -> 0)\n"
              << "  alu <op> <a> [b]  add sub and or xor (a b); not shl shr inc dec (a)\n"
              << "  regs              print PC A B H Z N C\n"
              << "  step              execute one instruction at PC, print its trace\n"
              << "  run               step until HALT or an error, print the step count\n"
              << "  trace <on|off>    also print a trace line per step in run (off)\n"
              << "  hostdump cpu      the host's CPU struct as raw bytes\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

int main() {
    Memory mem; // 4096 bytes, on the stack, zeroed by the {} in memory.hpp
    CPU cpu{&mem};
    bool trace = false; // `trace on`: run prints a line per step, like step

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
        } else if (cmd == "dump" && !second_word(line).empty()) {
            std::string a, n;
            long addr = 0, count = 16;
            words >> a;
            if (!parse_number(a, addr) || ((words >> n) && !parse_number(n, count))) {
                std::cout << "usage: dump [<addr> [count]]\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (static_cast<std::size_t>(addr) >= MEM_SIZE) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1 << '\n';
            } else if (count < 1 || static_cast<std::size_t>(count) > MEM_SIZE) {
                std::cout << "count is 1.." << MEM_SIZE << ", got " << count << '\n';
            } else {
                dump_range(mem, static_cast<std::size_t>(addr), static_cast<std::size_t>(count));
            }
        } else if (cmd == "dump") {
            dump(mem);
        } else if (cmd == "get" && past_the_box(second_word(line))) {
            // mem_get would quietly give 0 here; say so instead (Lab 3, notes §5).
            long addr = 0;
            parse_number(second_word(line), addr);
            std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1 << '\n';
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
        } else if (cmd == "get16") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: get16 <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (static_cast<std::size_t>(addr) >= MEM_SIZE - 1) {
                // get16 itself would read a 0 for the missing high byte.
                std::cout << "get16 needs two bytes at " << addr << " and "
                          << static_cast<std::size_t>(addr) + 1 << "; the box is 0.."
                          << MEM_SIZE - 1 << '\n';
            } else {
                const std::uint16_t v = get16(mem, static_cast<std::size_t>(addr));
                std::cout << v << "  0x" << std::hex << std::setfill('0') << std::setw(4) << v
                          << std::dec << std::setfill(' ') << '\n';
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
            } else if (!set16(mem, static_cast<std::size_t>(addr),
                              static_cast<std::uint16_t>(value))) {
                // Both cells must fit: the last valid start address is MEM_SIZE - 2.
                std::cout << "set16 needs two bytes at " << addr << " and "
                          << static_cast<std::size_t>(addr) + 1 << "; the box is 0.."
                          << MEM_SIZE - 1 << '\n';
            }
        } else if (cmd == "reg" && (second_word(line) == "pc" || second_word(line) == "h")) {
            // The given `reg` below sets the 8-bit A and B; PC and H are 16-bit.
            std::string r, v;
            long value = 0;
            words >> r;
            if (!(words >> v) || !parse_number(v, value) || value < 0 || value > 0xFFFF) {
                std::cout << "usage: reg <pc|h> <0..65535>\n";
            } else if (r == "pc") {
                cpu.pc = static_cast<std::uint16_t>(value);
            } else {
                cpu.h = static_cast<std::uint16_t>(value);
            }
        } else if (cmd == "reg") {
            std::string r, v;
            long value = 0;
            if (!(words >> r) || !(words >> v) || !parse_number(v, value) || value < 0 ||
                value > 255) {
                std::cout << "usage: reg <a|b> <0..255>\n";
            } else if (r == "a") {
                cpu.a = (Byte)value;
            } else if (r == "b") {
                cpu.b = (Byte)value;
            } else {
                std::cout << "unknown register: " << r << '\n';
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
            report(step(cpu, true), cpu);
        } else if (cmd == "run") {
            // Quiet unless `trace on`: the guest's output, then how many
            // instructions ran (HALT included).
            unsigned long steps = 0;
            StepResult r = step(cpu, trace);
            while (r == StepResult::ok) {
                ++steps;
                if (cpu.halted)
                    break;
                r = step(cpu, trace);
            }
            guest_end_line();
            if (r == StepResult::ok) {
                std::cout << "halted after " << steps << " steps\n";
            } else {
                report(r, cpu);
                if (r != StepResult::halted) // already halted: the refusal says it all
                    std::cout << "stopped after " << steps << " steps\n";
            }
        } else if (cmd == "trace") {
            std::string v;
            words >> v;
            if (v == "on")
                trace = true;
            else if (v == "off")
                trace = false;
            else
                std::cout << "usage: trace <on|off>\n";
        } else if (cmd == "hostdump") {
            std::string what;
            words >> what;
            if (what != "cpu") {
                std::cout << "usage: hostdump cpu\n";
            } else {
                // Host facts: where the struct lives in THIS process, and how big.
                std::cout << "&cpu=" << static_cast<const void*>(&cpu)
                          << "  sizeof(cpu)=" << sizeof(cpu)
                          << "  sizeof(cpu.mem)=" << sizeof(cpu.mem)
                          << "  sizeof(*cpu.mem)=" << sizeof(*cpu.mem) << '\n';
                dump(&cpu, sizeof(cpu));
            }
        } else {
            std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    return 0; // 0 means "success" to the operating system
}
