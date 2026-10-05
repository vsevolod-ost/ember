// main.cpp — the prompt.
//
// GIVEN, in full. This file is scaffolding: it reads a line, splits it into
// words, and calls one of your functions. It uses `while`, `if` and functions,
// which the course only explains properly in Labs 4 and 7. That is on purpose —
// in Lab 1 you read this file, you do not write it.
//
// What you add in later labs is one more `else if` branch per command.
#include <climits>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "alu.hpp"
#include "asm/assembler.hpp"
#include "asm/disasm.hpp"
#include "asm/expr.hpp"
#include "asm/lexer.hpp"
#include "asm/load.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "dump.hpp"
#include "guest_io.hpp"
#include "heap.hpp"
#include "hexdump.hpp"
#include "isa.hpp"
#include "layout.hpp"
#include "memory.hpp"
#include "messages.hpp"
#include "run.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "sprite.hpp"
#include "stack.hpp"
#include "value.hpp"

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
    else if (name == "mul")
        r = alu_mul(a, b, f);
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

// The kind named by a word of `value`; false for any other word.
static bool kind_by_name(const std::string& name, Value::Kind& kind) {
    if (name == "byte")
        kind = Value::Kind::Byte;
    else if (name == "addr")
        kind = Value::Kind::Addr;
    else if (name == "sprite")
        kind = Value::Kind::SpriteId;
    else
        return false;
    return true;
}

// One sprite's record, printed through a tagged Value like `value sprite i`.
static void print_sprite(int i, const Memory& mem, const Sprite sprites[]) {
    Value v{};
    if (make_value(Value::Kind::SpriteId, i, v))
        print_value(v, mem, sprites);
}

int main(int argc, char* argv[]) {
    // Lab 8: `./ember program.asm` assembles the file and runs it; with no
    // argument the REPL below opens, as it always did.
    if (argc == 2)
        return run_file(argv[1]);
    if (argc > 2) {
        std::cout << "usage: ember [program.asm]\n";
        return 2;
    }

    Memory mem; // 4096 bytes, on the stack, zeroed by the {} in memory.hpp
    CPU cpu{&mem};
    // Lab 6: eight host records; the {} makes every field 0, so all are dead.
    Sprite sprites[SPRITE_COUNT]{};
    Trace trace = Trace::off; // `trace on|calls`: run prints a line per step / per CALL and RET
    long breakpoint = -1;     // `b <addr>`: run stops when PC gets there; -1 = none
    Label* labels = nullptr;  // Lab 8: the labels of the last `asm`, for disasm and expr

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
        } else if (cmd == "reg" && second_word(line) == "sp") {
            // SP is 16 bits but only the stack region makes sense for it.
            std::string r, v, extra;
            long value = 0;
            words >> r;
            if (!(words >> v) || words >> extra || !parse_number(v, value) || value < 0 ||
                value > 0xFFFF) {
                std::cout << "usage: reg sp <" << hex(STACK_LO - 1, 4) << ".." << hex(STACK_HI, 4)
                          << ">\n";
            } else if (!stack_set_sp(cpu, static_cast<std::uint16_t>(value))) {
                std::cout << "SP is " << hex(STACK_LO - 1, 4) << ".." << hex(STACK_HI, 4)
                          << " (stack " << hex(STACK_LO, 4) << '-' << hex(STACK_HI, 4) << "), got "
                          << hex(static_cast<unsigned>(value), 4) << '\n';
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
            const bool two = name == "add" || name == "sub" || name == "and" || name == "or" ||
                             name == "xor" || name == "mul";
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
            // instructions ran (HALT included). The loop is in run.cpp, the
            // lines in messages.cpp (shared with the command line, Lab 8).
            report_run(run(cpu, trace, breakpoint), cpu);
        } else if (cmd == "trace") {
            std::string v;
            words >> v;
            if (v == "on")
                trace = Trace::on;
            else if (v == "off")
                trace = Trace::off;
            else if (v == "calls")
                trace = Trace::calls;
            else
                std::cout << "usage: trace <on|off|calls>\n";
        } else if (cmd == "b") {
            // One breakpoint; `b` and `b off` clear it. `step` ignores it.
            std::string a, extra;
            long addr = 0;
            words >> a;
            if (words >> extra) {
                std::cout << "usage: b <addr> | b off\n";
            } else if (a.empty() || a == "off") {
                breakpoint = -1;
                std::cout << "no breakpoint\n";
            } else if (!parse_number(a, addr)) {
                std::cout << "usage: b <addr> | b off\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (static_cast<std::size_t>(addr) >= MEM_SIZE) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1 << '\n';
            } else {
                breakpoint = addr;
                std::cout << "breakpoint at " << hex(static_cast<unsigned>(addr), 4) << '\n';
            }
        } else if (cmd == "find" && line.find('!') != std::string::npos) {
            // `find <lo> <hi> !<byte>`: the first byte that is NOT <byte>.
            // find_not is the same loop as find with != (search.cpp);
            // `find 0xA00 0xB00 !0` is where the picture on the screen starts.
            std::string l, h, n, extra;
            long lo = 0, hi = 0, needle = 0;
            if (!(words >> l >> h >> n) || words >> extra || n[0] != '!' || !parse_number(l, lo) ||
                !parse_number(h, hi) || !parse_number(n.substr(1), needle)) {
                std::cout << "usage: find <lo> <hi> !<byte>\n";
            } else if (lo < 0 || lo > hi || static_cast<std::size_t>(hi) > MEM_SIZE) {
                std::cout << "range must be 0 <= lo <= hi <= " << MEM_SIZE << '\n';
            } else if (needle < 0 || needle > 255) {
                std::cout << "a byte is 0..255, got " << needle << '\n';
            } else {
                const auto from = static_cast<std::uint16_t>(lo),
                           to = static_cast<std::uint16_t>(hi);
                const std::uint16_t i = find_not(mem, from, to, static_cast<Byte>(needle));
                if (i == to)
                    std::cout << "no byte != " << hex(static_cast<unsigned>(needle), 2) << " in ["
                              << hex(from, 4) << ", " << hex(to, 4) << ")\n";
                else
                    std::cout << "found " << hex(mem_get(mem, i), 2) << " at " << hex(i, 4)
                              << " (first byte != " << hex(static_cast<unsigned>(needle), 2)
                              << ")\n";
            }
        } else if (cmd == "find") {
            // The given C++ loop (search.cpp) over [lo, hi): hi itself is not
            // looked at, and hi == MEM_SIZE means "to the end of the box".
            std::string l, h, n, extra;
            long lo = 0, hi = 0, needle = 0;
            if (!(words >> l >> h >> n) || words >> extra || !parse_number(l, lo) ||
                !parse_number(h, hi) || !parse_number(n, needle)) {
                std::cout << "usage: find <lo> <hi> <byte>\n";
            } else if (lo < 0 || lo > hi || static_cast<std::size_t>(hi) > MEM_SIZE) {
                std::cout << "range must be 0 <= lo <= hi <= " << MEM_SIZE << '\n';
            } else if (needle < 0 || needle > 255) {
                std::cout << "a byte is 0..255, got " << needle << '\n';
            } else {
                const auto from = static_cast<std::uint16_t>(lo),
                           to = static_cast<std::uint16_t>(hi);
                const std::uint16_t i = find(mem, from, to, static_cast<Byte>(needle));
                if (i == to)
                    std::cout << hex(static_cast<unsigned>(needle), 2) << " not found in ["
                              << hex(from, 4) << ", " << hex(to, 4) << ")\n";
                else
                    std::cout << "found " << hex(static_cast<unsigned>(needle), 2) << " at "
                              << hex(i, 4) << '\n';
            }
        } else if (cmd == "show") {
            show(mem); // the given show(), the same one SHOW calls
        } else if (cmd == "cls") {
            clear(mem);
        } else if (cmd == "plot" || cmd == "unplot") {
            // The same plot/unplot as PLOT/UNPLOT; a refused pixel changes nothing.
            std::string xs, ys, extra;
            long x = 0, y = 0;
            if (!(words >> xs >> ys) || words >> extra || !parse_number(xs, x) ||
                !parse_number(ys, y)) {
                std::cout << "usage: " << cmd << " <x> <y>\n";
            } else {
                // A number too big for an int is off the screen too.
                const bool fits = x >= INT_MIN && x <= INT_MAX && y >= INT_MIN && y <= INT_MAX;
                const int xi = fits ? static_cast<int>(x) : -1,
                          yi = fits ? static_cast<int>(y) : -1;
                const bool done = cmd == "plot" ? plot(mem, xi, yi) : unplot(mem, xi, yi);
                if (!done)
                    std::cout << "pixel (" << x << ", " << y << ") is outside the " << SCREEN_W
                              << 'x' << SCREEN_H << " screen; nothing changed\n";
            }
        } else if (cmd == "frame") {
            frame(mem);
        } else if (cmd == "rowmin") {
            std::string ys, extra;
            long y = 0;
            if (!(words >> ys) || words >> extra || !parse_number(ys, y)) {
                std::cout << "usage: rowmin <y>\n";
            } else if (y < 0 || y >= SCREEN_H) {
                std::cout << "row is 0.." << SCREEN_H - 1 << ", got " << y << '\n';
            } else {
                const int x = rowmin(mem, static_cast<int>(y));
                if (x < 0)
                    std::cout << "row " << y << ": no lit pixels\n";
                else
                    std::cout << "row " << y << ": leftmost lit pixel at x=" << x << '\n';
            }
        } else if (cmd == "sort") {
            // In place over [lo, hi), like find's range. The loops are in sort.cpp.
            std::string l, h, opt;
            long lo = 0, hi = 0;
            SortKind kind = SortKind::bubble;
            bool draw = false, named = false; // each option at most once
            bool bad = !(words >> l >> h) || !parse_number(l, lo) || !parse_number(h, hi);
            while (!bad && words >> opt) {
                if ((opt == "bubble" || opt == "insert") && !named) {
                    kind = opt == "bubble" ? SortKind::bubble : SortKind::insert;
                    named = true;
                } else if (opt == "show" && !draw) {
                    draw = true;
                } else {
                    bad = true;
                }
            }
            if (bad) {
                std::cout << "usage: sort <lo> <hi> [bubble|insert] [show]\n";
            } else if (lo < 0 || lo > hi || static_cast<std::size_t>(hi) > MEM_SIZE) {
                std::cout << "range must be 0 <= lo <= hi <= " << MEM_SIZE << '\n';
            } else if (draw && (hi - lo < 1 || hi - lo > SCREEN_W)) {
                // One column per byte: at most one pixel wide each.
                std::cout << "sort with show needs 1.." << SCREEN_W << " bytes, got " << hi - lo
                          << '\n';
            } else if (draw && static_cast<std::size_t>(lo) <= VRAM_HI &&
                       static_cast<std::size_t>(hi) > VRAM_LO) {
                // The columns are drawn into VRAM: they would overwrite the bytes.
                std::cout << "the range overlaps the screen; sort it without show\n";
            } else {
                const auto from = static_cast<std::size_t>(lo), to = static_cast<std::size_t>(hi);
                const SortStats st = sort_range(mem, from, to, kind, draw);
                std::cout << "sorted [" << hex(static_cast<unsigned>(from), 4) << ", "
                          << hex(static_cast<unsigned>(to), 4) << ") by "
                          << (kind == SortKind::bubble ? "bubble" : "insert") << ": " << st.passes
                          << " passes, " << st.compares << " compares, " << st.swaps << " swaps\n";
            }
        } else if (cmd == "anim") {
            // A game loop for the poor: run at most `steps` steps (the same
            // run(), so `b` and `trace` count), show the screen, again. With
            // `b` at the head of the guest's loop a frame is one pass of it.
            std::string s, f, extra;
            long steps = 0, frames = 0;
            if (!(words >> s >> f) || words >> extra || !parse_number(s, steps) ||
                !parse_number(f, frames) || steps < 1 ||
                static_cast<unsigned long>(steps) > RUN_STEP_LIMIT || frames < 1 || frames > 1000) {
                std::cout << "usage: anim <steps 1.." << RUN_STEP_LIMIT << "> <frames 1..1000>\n";
            } else {
                for (long k = 1; k <= frames; ++k) {
                    const RunResult r =
                        run(cpu, trace, breakpoint, static_cast<unsigned long>(steps));
                    guest_end_line();
                    if (r.why == RunStop::halted) {
                        std::cout << "halted after " << r.steps << " steps\n";
                        break;
                    }
                    if (r.why == RunStop::error) {
                        report(r.error, cpu);
                        if (r.error != StepResult::halted)
                            std::cout << "stopped after " << r.steps << " steps\n";
                        break;
                    }
                    std::cout << "frame " << k << ": " << r.steps << " steps, PC=" << hex(cpu.pc, 4)
                              << '\n';
                    show(mem);
                }
            }
        } else if (cmd == "hostdump" && second_word(line) == "sprites") {
            // Eight 5-byte records one after another: an array has no gaps.
            std::cout << "&sprites=" << static_cast<const void*>(sprites)
                      << "  sizeof(sprites)=" << sizeof(sprites)
                      << "  sizeof(Sprite)=" << sizeof(Sprite) << '\n';
            dump(sprites, sizeof(sprites));
        } else if (cmd == "hostdump") {
            std::string what;
            words >> what;
            if (what != "cpu") {
                std::cout << "usage: hostdump <cpu|sprites>\n";
            } else {
                // Host facts: where the struct lives in THIS process, and how big.
                std::cout << "&cpu=" << static_cast<const void*>(&cpu)
                          << "  sizeof(cpu)=" << sizeof(cpu)
                          << "  sizeof(cpu.mem)=" << sizeof(cpu.mem)
                          << "  sizeof(*cpu.mem)=" << sizeof(*cpu.mem) << '\n';
                dump(&cpu, sizeof(cpu));
            }
        } else if (cmd == "layout") {
            std::string what, extra;
            words >> what;
            if (words >> extra || !print_layout(what))
                std::cout << "usage: layout [cpu|flags|instruction|sprite|value]\n";
        } else if (cmd == "sprite") {
            // `sprite <i> <x> <y> <vx> <vy>` or `sprite <i> off`. Every range
            // is checked before the record changes.
            std::string w[5], extra;
            long n[5] = {0, 0, 0, 0, 0};
            int got = 0;
            while (got < 5 && words >> w[got])
                ++got;
            const bool off = got == 2 && w[1] == "off";
            bool numbers = got == 5 || off;
            for (int k = 0; numbers && k < (off ? 1 : 5); ++k)
                numbers = parse_number(w[k], n[k]);
            if (!numbers || words >> extra) {
                std::cout << "usage: sprite <i> <x> <y> <vx> <vy> | sprite <i> off\n";
            } else if (n[0] < 0 || n[0] >= SPRITE_COUNT) {
                print_value_error(Value::Kind::SpriteId, n[0]);
            } else if (!off && (n[1] < 0 || n[1] >= SCREEN_W)) {
                std::cout << "x is 0.." << SCREEN_W - 1 << ", got " << n[1] << '\n';
            } else if (!off && (n[2] < 0 || n[2] >= SCREEN_H)) {
                std::cout << "y is 0.." << SCREEN_H - 1 << ", got " << n[2] << '\n';
            } else if (!off && (n[3] < -(SCREEN_W - 1) || n[3] > SCREEN_W - 1)) {
                std::cout << "vx is " << -(SCREEN_W - 1) << ".." << SCREEN_W - 1 << ", got " << n[3]
                          << '\n';
            } else if (!off && (n[4] < -(SCREEN_H - 1) || n[4] > SCREEN_H - 1)) {
                std::cout << "vy is " << -(SCREEN_H - 1) << ".." << SCREEN_H - 1 << ", got " << n[4]
                          << '\n';
            } else {
                Sprite& s = sprites[n[0]];
                if (off) {
                    s.alive = false;
                } else {
                    // All in range: the casts cut nothing.
                    s.x = static_cast<std::uint8_t>(n[1]);
                    s.y = static_cast<std::uint8_t>(n[2]);
                    s.vx = static_cast<std::int8_t>(n[3]);
                    s.vy = static_cast<std::int8_t>(n[4]);
                    s.alive = true;
                }
                sprites_draw(mem, sprites);
                print_sprite(static_cast<int>(n[0]), mem, sprites);
            }
        } else if (cmd == "sprites") {
            for (int i = 0; i < SPRITE_COUNT; ++i)
                print_sprite(i, mem, sprites);
        } else if (cmd == "tick") {
            std::string t, extra;
            long times = 1;
            if ((words >> t && !parse_number(t, times)) || words >> extra || times < 1 ||
                times > 100) {
                std::cout << "usage: tick [1..100]\n";
            } else {
                // The screen is a picture of the array: move, then redraw it all.
                for (long k = 1; k <= times; ++k) {
                    sprites_tick(sprites);
                    sprites_draw(mem, sprites);
                    std::cout << "tick " << k << ": " << sprites_alive(sprites) << " alive\n";
                    show(mem);
                }
            }
        } else if (cmd == "stack") {
            std::string ns, extra;
            long n = 8;
            if ((words >> ns && !parse_number(ns, n)) || words >> extra || n < 1 ||
                static_cast<std::size_t>(n) > STACK_HI + 1 - STACK_LO) {
                std::cout << "usage: stack [1.." << STACK_HI + 1 - STACK_LO << "]\n";
            } else {
                stack_print(cpu, static_cast<std::size_t>(n));
            }
        } else if (cmd == "heap") {
            heap_print(cpu);
        } else if (cmd == "alloc") {
            // The same heap_alloc as ALLOC, but the host is a debugger: H and
            // C stay as they are; only the heap changes.
            std::string ns, extra;
            long n = 0;
            if (!(words >> ns) || words >> extra || !parse_number(ns, n) || n < 0 || n > 255) {
                std::cout << "usage: alloc <0..255>\n";
            } else {
                std::uint16_t at = 0;
                bool reused = false;
                const HeapResult r = heap_alloc(cpu, static_cast<Byte>(n), at, reused);
                if (r == HeapResult::ok)
                    std::cout << "allocated " << n << " bytes at " << hex(at, 4)
                              << (reused ? " (reused a free block)" : " (bump)")
                              << ", heap_ptr=" << hex(cpu.heap_ptr, 4) << '\n';
                else if (r == HeapResult::no_room)
                    std::cout << n << " bytes do not fit: heap_ptr=" << hex(cpu.heap_ptr, 4) << ", "
                              << HEAP_HI + 1 - cpu.heap_ptr << " left; nothing done\n";
                else
                    print_heap_refusal("not allocated", r, cpu);
            }
        } else if (cmd == "free") {
            std::string as, ns, extra;
            long addr = 0, n = 0;
            if (!(words >> as >> ns) || words >> extra || !parse_number(as, addr) ||
                !parse_number(ns, n) || addr < 0 || addr > 0xFFFF || n < 0 || n > 255) {
                std::cout << "usage: free <addr> <0..255>\n";
            } else {
                bool top = false;
                const HeapResult r =
                    heap_free(cpu, static_cast<std::uint16_t>(addr), static_cast<Byte>(n), top);
                if (r == HeapResult::ok && top)
                    std::cout << "freed " << n << " bytes at "
                              << hex(static_cast<unsigned>(addr), 4)
                              << " (top: heap_ptr=" << hex(cpu.heap_ptr, 4) << ")\n";
                else if (r == HeapResult::ok)
                    std::cout << "freed " << n << " bytes at "
                              << hex(static_cast<unsigned>(addr), 4) << " (free list)\n";
                else
                    print_heap_refusal("not freed", r, cpu);
            }
        } else if (cmd == "value") {
            std::string k, ns, extra;
            long n = 0;
            Value::Kind kind = Value::Kind::Byte;
            Value v{};
            if (!(words >> k >> ns) || words >> extra || !kind_by_name(k, kind) ||
                !parse_number(ns, n)) {
                std::cout << "usage: value <byte|addr|sprite> <n>\n";
            } else if (!make_value(kind, n, v)) {
                print_value_error(kind, n);
            } else {
                print_value(v, mem, sprites);
            }
        } else if (cmd == "lex") {
            // The given lexer on a whole file: one token per line, or the error.
            std::string path, extra, src, error;
            if (!(words >> path) || words >> extra) {
                std::cout << "usage: lex <file.asm>\n";
            } else if (!read_file(path, src)) {
                std::cout << "cannot open " << path << '\n';
            } else if (Token* tokens = lex(src, error)) {
                print_tokens(tokens);
                destroy(tokens);
            } else {
                std::cout << error << '\n'; // lex has already freed the list
            }
        } else if (cmd == "asm") {
            // Assemble, print the listing, load the bytes, then PC = 0 with
            // an empty stack and heap (load.cpp). A, B, H and the rest of
            // memory stay: `set 1 10` after it changes the program's n.
            std::string path, mode, extra, src, error;
            words >> path >> mode;
            if (path.empty() || (!mode.empty() && mode != "onepass") || words >> extra) {
                std::cout << "usage: asm <file.asm> [onepass]\n";
            } else if (!read_file(path, src)) {
                std::cout << "cannot open " << path << '\n';
            } else {
                Program* p = new Program{}; // 20 KB: on the heap
                if (!assemble(src, mode.empty() ? AsmMode::two_pass : AsmMode::one_pass, *p,
                              error)) {
                    std::cout << error << "; nothing loaded\n";
                } else {
                    print_listing(*p, src);
                    load_program(*p, mem);
                    start_program(cpu);
                    labels_destroy(labels); // the labels are this program's now
                    labels = p->labels;
                    p->labels = nullptr;
                    program_free(*p);
                }
                delete p;
            }
        } else if (cmd == "disasm") {
            // Ranges [lo, hi) like find; several in one go come out as one
            // source text, with .org between them, so no byte may be in two.
            const std::size_t MAX_RANGES = 8;
            std::uint16_t lo[MAX_RANGES], hi[MAX_RANGES];
            std::size_t n = 0;
            bool bad = false, outside = false, overlap = false;
            std::string l, h;
            while (!bad && words >> l) {
                long from = 0, to = 0;
                if (n == MAX_RANGES || !(words >> h) || !parse_number(l, from) ||
                    !parse_number(h, to)) {
                    bad = true;
                } else if (from < 0 || from > to || static_cast<std::size_t>(to) > MEM_SIZE) {
                    outside = true;
                } else {
                    lo[n] = static_cast<std::uint16_t>(from);
                    hi[n] = static_cast<std::uint16_t>(to);
                    for (std::size_t i = 0; i < n; ++i)
                        if (lo[i] < hi[n] && lo[n] < hi[i])
                            overlap = true;
                    ++n;
                }
            }
            if (bad || (n == 0 && !outside))
                std::cout << "usage: disasm <lo> <hi> [<lo> <hi> ...] (at most " << MAX_RANGES
                          << " ranges)\n";
            else if (outside)
                std::cout << "range must be 0 <= lo <= hi <= " << MEM_SIZE << '\n';
            else if (overlap)
                std::cout << "ranges must not overlap\n";
            else
                disasm_ranges(mem, lo, hi, n, labels);
        } else if (cmd == "expr") {
            // The rest of the line, through the lexer and the expression
            // parser of the assembler: the tree, then its value.
            std::string text, error;
            std::getline(words, text);
            Token* tokens = lex(text, error);
            const Token* t = tokens;
            Expr* e = nullptr;
            long v = 0;
            if (!tokens) {
                std::cout << error << '\n';
            } else if (t->kind == Token::Kind::Eof) {
                std::cout << "usage: expr <expression>\n";
            } else if (!(e = parse_expr(t, error))) {
                std::cout << error << '\n';
            } else if (t->kind != Token::Kind::Eof) {
                std::cout << "line 1: unexpected " << token_word(t) << " after " << expr_text(e)
                          << '\n';
            } else {
                expr_print(e);
                if (expr_eval(e, labels, v, error) != EvalResult::ok)
                    std::cout << error << '\n';
                else if (v < 0)
                    std::cout << "= " << v << '\n';
                else
                    std::cout << "= " << v << "  (" << hex(static_cast<unsigned>(v), 4) << ")\n";
            }
            expr_destroy(e);
            destroy(tokens);
        } else if (cmd == "isa") {
            // The one table step() and the assembler read (isa.cpp).
            std::size_t count = 0;
            const IsaRow* rows = isa_rows(count);
            for (std::size_t i = 0; i < count; ++i)
                std::cout << hex(static_cast<unsigned>(rows[i].op), 2) << "  " << row_size(rows[i])
                          << "  " << rows[i].text << '\n';
            std::cout << count << " rows\n";
        } else {
            std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    stack_clear(cpu.stack); // the list build gives its nodes back
    labels_destroy(labels);
    return 0; // 0 means "success" to the operating system
}
