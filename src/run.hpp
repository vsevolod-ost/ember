// run.hpp — `run`: step until something stops the machine.
#pragma once

#include "cpu.hpp"

// A program that never reaches HALT (JMP to itself) must not hang the REPL.
// Counted per `run` command; the next `run` gets the same budget again.
const unsigned long RUN_STEP_LIMIT = 100000;

enum class RunStop {
    halted,     // HALT executed (counted in steps)
    error,      // a step was refused; RunResult::error says why
    breakpoint, // PC reached the breakpoint before that instruction ran
    step_limit, // RUN_STEP_LIMIT steps and no HALT; the machine is not halted
};

struct RunResult {
    RunStop why;
    StepResult error;    // StepResult::ok unless why == RunStop::error
    unsigned long steps; // instructions executed by this run
};

// Step from PC until HALT, an error, the breakpoint (a guest address, < 0
// for none) or RUN_STEP_LIMIT steps. With `trace`, one line per step. The
// breakpoint is not checked before the first step, so `run` from a
// breakpoint goes on, like "continue" in a debugger.
RunResult run(CPU& cpu, bool trace, long breakpoint);
