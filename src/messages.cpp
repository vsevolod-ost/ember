// messages.cpp — what the REPL prints about the machine: why a step was
// refused, why the heap said no, and `help`. Moved out of main.cpp in Lab 7
// (texts unchanged) so main.cpp is the loop and the parsing of commands.
#include "messages.hpp"

#include <iostream>

#include "guest_io.hpp"
#include "run.hpp"
#include "sprite.hpp"
#include "stack.hpp"

void report(StepResult r, const CPU& cpu) {
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
    case StepResult::stack_overflow:
    case StepResult::stack_underflow: {
        // Nothing changed: SP and the stack are as the instruction found them.
        const std::uint16_t need = stack_bytes(cpu);
        const bool over = r == StepResult::stack_overflow;
        std::cout << (over ? "stack overflow: " : "stack underflow: ") << disasm(*cpu.mem, cpu.pc)
                  << " at " << hex(cpu.pc, 4) << " needs " << need
                  << (need == 1 ? " byte, " : " bytes, ");
        if (over)
            std::cout << stack_free(cpu.stack) << " free (SP=" << hex(stack_sp(cpu.stack), 4)
                      << ", stack " << hex(STACK_LO, 4) << '-' << hex(STACK_HI, 4) << ")";
        else
            std::cout << stack_used(cpu.stack)
                      << " on the stack (SP=" << hex(stack_sp(cpu.stack), 4) << ")";
        std::cout << "; nothing done\n";
        break;
    }
    }
}

void report_run(const RunResult& r, const CPU& cpu) {
    guest_end_line();
    if (r.why == RunStop::halted) {
        std::cout << "halted after " << r.steps << " steps\n";
    } else if (r.why == RunStop::error) {
        report(r.error, cpu);
    } else if (r.why == RunStop::breakpoint) {
        std::cout << "breakpoint at " << hex(cpu.pc, 4) << '\n';
    } else {
        std::cout << "step limit " << RUN_STEP_LIMIT << " reached at " << hex(cpu.pc, 4) << '\n';
    }
    // Already halted before this run: the refusal says it all.
    if (r.why != RunStop::halted && r.error != StepResult::halted)
        std::cout << "stopped after " << r.steps << " steps\n";
}

// Why heap_alloc/heap_free said no, for `alloc` and `free`: "not freed: ...".
void print_heap_refusal(const char* what, HeapResult r, const CPU& cpu) {
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

void print_help() {
    std::cout << "commands:\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  dump <addr> [n]   print the rows holding n bytes from addr (n=16)\n"
              << "  get <addr>        show one byte four ways\n"
              << "  get16 <addr>      read a 16-bit value, little-endian\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  set16 <addr> <v>  write a 16-bit value, little-endian\n"
              << "  reg <a|b> <v>     put 0..255 into register A or B\n"
              << "  reg <pc|h> <v>    put 0..65535 into PC or H\n"
              << "  reg sp <v>        put " << hex(STACK_LO - 1, 4) << ".." << hex(STACK_HI, 4)
              << " into SP (" << hex(STACK_HI, 4) << " = empty stack)\n"
              << "  inc <addr>        add one to a byte (wraps 255 -> 0)\n"
              << "  alu <op> <a> [b]  add sub and or xor mul (a b); not shl shr inc dec (a)\n"
              << "  regs              print PC A B H SP Z N C\n"
              << "  step              execute one instruction at PC, print its trace\n"
              << "  run               step until HALT, an error, the breakpoint or "
              << RUN_STEP_LIMIT << " steps\n"
              << "  trace <on|off|calls>\n"
              << "                    also print a trace line per step in run (off);\n"
              << "                    calls: only for CALL and RET\n"
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
              << "  stack [n]         guest stack " << hex(STACK_LO, 4) << '-' << hex(STACK_HI, 4)
              << ": SP, deepest SP, n newest bytes (8)\n"
              << "  heap              guest heap " << hex(HEAP_LO, 4) << '-' << hex(HEAP_HI, 4)
              << ": heap_ptr and the free list\n"
              << "  alloc <n>         ALLOC n bytes (0..255) as the host; registers untouched\n"
              << "  free <addr> <n>   FREE n bytes at addr as the host; registers untouched\n"
              << "  value <byte|addr|sprite> <n>\n"
              << "                    make a tagged Value and print it by its tag\n"
              << "  lex <file.asm>    print the tokens of a file, one per line\n"
              << "  asm <file.asm> [onepass]\n"
              << "                    assemble a file: listing, labels; load it, PC=0x0000,\n"
              << "                    SP=0x0fff, empty heap; onepass: one pass, forward\n"
              << "                    labels patched from a queue\n"
              << "  disasm <lo> <hi> [<lo> <hi> ...]\n"
              << "                    bytes of [lo, hi) as instructions, labels of the last asm\n"
              << "  expr <expression> parse an operand expression: its tree and value\n"
              << "  isa               the instruction table: opcode, size, mnemonic\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}
