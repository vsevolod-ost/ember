// run.cpp — the loop behind `run`. It only counts and decides when to stop;
// main.cpp prints the reason.
#include "run.hpp"

// `trace calls`: does the instruction at PC call or return? Asked before the
// step, so a refused CALL prints no line, like any refused step.
static bool calls_or_returns(const CPU& cpu) {
    const Byte op = mem_get(*cpu.mem, cpu.pc);
    return op == static_cast<Byte>(Op::Call) || op == static_cast<Byte>(Op::Ret);
}

RunResult run(CPU& cpu, Trace trace, long breakpoint, unsigned long limit) {
    unsigned long steps = 0;
    while (true) {
        if (steps > 0 && cpu.pc == breakpoint)
            return {RunStop::breakpoint, StepResult::ok, steps};
        if (steps == limit)
            return {RunStop::step_limit, StepResult::ok, steps};
        const bool print = trace == Trace::on || (trace == Trace::calls && calls_or_returns(cpu));
        const StepResult r = step(cpu, print);
        if (r != StepResult::ok)
            return {RunStop::error, r, steps};
        ++steps;
        if (cpu.halted)
            return {RunStop::halted, StepResult::ok, steps};
    }
}
