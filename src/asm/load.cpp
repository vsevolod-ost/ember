// load.cpp — reading a file, and the command line `./ember program.asm`.
#include "load.hpp"

#include <cstdio>
#include <iostream>

#include "assembler.hpp"
#include "cpu.hpp"
#include "messages.hpp"
#include "run.hpp"
#include "stack.hpp"

bool read_file(const std::string& path, std::string& text) {
    // C stdio, because it tells "end of file" from "could not read": a
    // directory opens fine and then fails on the first read (ferror).
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;
    std::string all;
    char chunk[4096];
    std::size_t got = 0;
    while ((got = std::fread(chunk, 1, sizeof(chunk), f)) > 0)
        all.append(chunk, got);
    const bool ok = !std::ferror(f);
    std::fclose(f);
    if (ok)
        text = all;
    return ok;
}

int run_file(const std::string& path) {
    std::string src, error;
    if (!read_file(path, src)) {
        std::cout << path << ": cannot open\n";
        return 1;
    }
    // A Program is 20 KB: on the heap, like the token nodes.
    Program* p = new Program{};
    if (!assemble(src, AsmMode::two_pass, *p, error)) {
        delete p;
        std::cout << path << ": " << error << '\n';
        return 1;
    }
    Memory mem; // a fresh box, zeroed, like the REPL's
    CPU cpu{&mem};
    load_program(*p, mem);
    program_free(*p);
    delete p;

    const RunResult r = run(cpu, Trace::off, -1);
    report_run(r, cpu);
    stack_clear(cpu.stack); // the list build gives its nodes back
    return r.why == RunStop::halted ? 0 : 1;
}

void start_program(CPU& cpu) {
    cpu.pc = 0;
    cpu.halted = false;
    // Like `reg sp 0xfff`: the list build gives its nodes back as well.
    stack_set_sp(cpu, static_cast<std::uint16_t>(STACK_HI));
    cpu.heap_ptr = HEAP_LO;
    cpu.free_head = 0;
}
