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
#include "cpu.hpp"
#include "display.hpp"
#include "dump.hpp"
#include "guest_io.hpp"
#include "heap.hpp"
#include "hexdump.hpp"
#include "layout.hpp"
#include "memory.hpp"
#include "run.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "sprite.hpp"
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
    case StepResult::bad_heap: {
        // Nothing changed, so the same walk finds the same bad node.
        std::uint16_t bad = 0;
        heap_check(cpu, bad);
        std::cout << "free list is broken at " << hex(bad, 4) << " (" << disasm(*cpu.mem, cpu.pc)
                  << " at " << hex(cpu.pc, 4) << "); nothing done\n";
        break;
    }
    }
}

// Why heap_alloc/heap_free said no, for `alloc` and `free`: "not freed: ...".
static void print_heap_refusal(const char* what, HeapResult r, const CPU& cpu) {
    std::uint16_t bad = 0;
    switch (r) {
    case HeapResult::ok:
    case HeapResult::no_room:
        break;
    case HeapResult::bad_block:
        std::cout << what << ": not a used heap block; nothing done\n";
        break;
    case HeapResult::already_free:
        std::cout << what << ": already free; nothing done\n";
        break;
    case HeapResult::too_small:
        std::cout << what << ": too small for a list node (" << NODE_SIZE
                  << " bytes); nothing done\n";
        break;
    case HeapResult::broken_list:
        heap_check(cpu, bad);
        std::cout << what << ": free list is broken at " << hex(bad, 4) << "; nothing done\n";
        break;
    }
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
              << "  run               step until HALT, an error, the breakpoint or "
              << RUN_STEP_LIMIT << " steps\n"
              << "  trace <on|off>    also print a trace line per step in run (off)\n"
              << "  b <addr> | b off  stop run before the instruction at addr; b alone = off\n"
              << "  find <lo> <hi> <byte>\n"
              << "                    first address in [lo, hi) that holds byte\n"
              << "  find <lo> <hi> !<byte>\n"
              << "                    first address in [lo, hi) that does not hold byte\n"
              << "  show              draw the screen: VRAM 0xA00-0xAFF, 64x32, # = lit\n"
              << "  cls               turn every pixel off (256 zero bytes)\n"
              << "  plot <x> <y>      light a pixel, x 0..63, y 0..31\n"
              << "  unplot <x> <y>    turn a pixel off\n"
              << "  frame             draw a border around the screen\n"
              << "  rowmin <y>        the leftmost lit pixel in row y\n"
              << "  sort <lo> <hi> [bubble|insert] [show]\n"
              << "                    sort the bytes of [lo, hi) in place (bubble);\n"
              << "                    show: draw them as columns after every pass\n"
              << "  anim <steps> <frames>\n"
              << "                    run at most steps steps, then show; frames times\n"
              << "  hostdump <cpu|sprites>\n"
              << "                    the host's CPU struct or sprites array as raw bytes\n"
              << "  layout [cpu|flags|instruction|sprite|value]\n"
              << "                    offset and size of each field, padding, sizeof\n"
              << "  sprite <i> <x> <y> <vx> <vy>\n"
              << "                    set host sprite i (0..7) alive: x 0..63, y 0..31,\n"
              << "                    vx -63..63, vy -31..31; redraw VRAM\n"
              << "  sprite <i> off    kill sprite i; redraw VRAM\n"
              << "  sprites           list all " << SPRITE_COUNT << " sprites\n"
              << "  tick [n]          n times (1..100, 1): move alive sprites, bounce, show\n"
              << "  heap              guest heap " << hex(HEAP_LO, 4) << '-' << hex(HEAP_HI, 4)
              << ": heap_ptr and the free list\n"
              << "  alloc <n>         ALLOC n bytes (0..255) as the host; registers untouched\n"
              << "  free <addr> <n>   FREE n bytes at addr as the host; registers untouched\n"
              << "  value <byte|addr|sprite> <n>\n"
              << "                    make a tagged Value and print it by its tag\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

int main() {
    Memory mem; // 4096 bytes, on the stack, zeroed by the {} in memory.hpp
    CPU cpu{&mem};
    // Lab 6: eight host records; the {} makes every field 0, so all are dead.
    Sprite sprites[SPRITE_COUNT]{};
    bool trace = false;   // `trace on`: run prints a line per step, like step
    long breakpoint = -1; // `b <addr>`: run stops when PC gets there; -1 = none

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
            // instructions ran (HALT included). The loop is in run.cpp.
            const RunResult r = run(cpu, trace, breakpoint);
            guest_end_line();
            if (r.why == RunStop::halted) {
                std::cout << "halted after " << r.steps << " steps\n";
            } else if (r.why == RunStop::error) {
                report(r.error, cpu);
            } else if (r.why == RunStop::breakpoint) {
                std::cout << "breakpoint at " << hex(cpu.pc, 4) << '\n';
            } else {
                std::cout << "step limit " << RUN_STEP_LIMIT << " reached at " << hex(cpu.pc, 4)
                          << '\n';
            }
            // Already halted before this run: the refusal says it all.
            if (r.why != RunStop::halted && r.error != StepResult::halted)
                std::cout << "stopped after " << r.steps << " steps\n";
        } else if (cmd == "trace") {
            std::string v;
            words >> v;
            if (v == "on")
                trace = true;
            else if (v == "off")
                trace = false;
            else
                std::cout << "usage: trace <on|off>\n";
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
        } else {
            std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    return 0; // 0 means "success" to the operating system
}
