// run.cpp — the loop behind `run`. It only counts and decides when to stop;
// main.cpp prints the reason.
#include "run.hpp"

RunResult run(CPU& cpu, bool trace, long breakpoint) {
    unsigned long steps = 0;
    while (true) {
        if (steps > 0 && cpu.pc == breakpoint)
            return {RunStop::breakpoint, StepResult::ok, steps};
        if (steps == RUN_STEP_LIMIT)
            return {RunStop::step_limit, StepResult::ok, steps};
        const StepResult r = step(cpu, trace);
        if (r != StepResult::ok)
            return {RunStop::error, r, steps};
        ++steps;
        if (cpu.halted)
            return {RunStop::halted, StepResult::ok, steps};
    }
}
