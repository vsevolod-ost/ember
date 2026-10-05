// load.hpp — from a file name to a running program (Lab 8 §4):
// .asm -> lexer -> tokens -> assembler -> bytes in Memory -> run.
#pragma once

#include <string>

struct CPU;

// The whole file as text. False if it cannot be opened or read.
bool read_file(const std::string& path, std::string& text);

// `./ember program.asm`: assemble (two passes) into a fresh machine and run
// it quietly, like `run` with `trace off`: the guest's output, then
// "halted after N steps". Returns the exit status: 0 after HALT; 1 if the
// file cannot be read or assembled ("<path>: line N: ...", nothing runs)
// or a step was refused or the step limit hit (the message `run` prints).
int run_file(const std::string& path);

// After `asm` has loaded a program into the REPL's machine: what a program
// starts with and cannot set up itself (ISA §1) — PC = 0x000, not halted,
// SP = 0xFFF with an empty stack (the deepest mark too), an empty heap. So
// a program that a breakpoint or an error stopped mid-CALL leaves nothing
// behind for the next one. A, B, H, the flags and the rest of memory stay:
// `set 1 10` after `asm programs/fib.asm` is how fib gets another n.
void start_program(CPU& cpu);
