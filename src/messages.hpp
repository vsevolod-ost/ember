// messages.hpp — the REPL's texts about the machine (Lab 7: out of main.cpp).
#pragma once

#include "cpu.hpp"
#include "heap.hpp"

// One line: why the step at PC was refused (nothing for StepResult::ok).
// Ends the guest's open line first.
void report(StepResult r, const CPU& cpu);

// Why heap_alloc/heap_free said no, for `alloc` and `free`: "not freed: ...".
void print_heap_refusal(const char* what, HeapResult r, const CPU& cpu);

// `help`: every command, one or two lines each.
void print_help();
