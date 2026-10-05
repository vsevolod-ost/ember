// cpu.cpp — fetch mem[pc], decode, execute, advance pc by the ISA size (or jump).
#include "cpu.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "display.hpp"
#include "guest_io.hpp"
#include "heap.hpp"
#include "opcodes.hpp"

Decoded decode(Byte op) {
    return {static_cast<Byte>((op >> 4) & 0x0F), static_cast<Byte>(op & 0x0F)};
}

// The "розмір" column of the ISA table, in one place.
std::uint16_t instr_size(Byte op) {
    switch (static_cast<Op>(op)) {
    case Op::Halt:
    case Op::Nop:
    case Op::Out:
    case Op::Outn:
    case Op::Cls:
    case Op::Plot:
    case Op::Unplot:
    case Op::Show:
    case Op::Add:
    case Op::Sub:
    case Op::And:
    case Op::Or:
    case Op::Xor:
    case Op::Not:
    case Op::Shl:
    case Op::Shr:
    case Op::Inc:
    case Op::Dec:
    case Op::Cmp:
    case Op::MovAB:
    case Op::MovBA:
    case Op::LoadAH:
    case Op::StoreHA:
    case Op::IncH:
    case Op::DecH:
    case Op::HLow:
    case Op::Alloc:
    case Op::Free:
        return 1;
    case Op::LoadiA:
    case Op::LoadiB:
        return 2; // opcode, imm8
    case Op::Outs:
    case Op::LoadA:
    case Op::LoadB:
    case Op::StoreA:
    case Op::StoreB:
    case Op::LoadH:
    case Op::Jmp:
    case Op::Jz:
    case Op::Jnz:
    case Op::Jc:
    case Op::Jnc:
    case Op::Jg:
    case Op::Jl:
        return 3; // opcode, then a 16-bit operand low byte first
    default:
        return 0; // no ISA row (yet): the first place an unknown opcode is caught
    }
}

std::string hex(unsigned v, int digits) {
    std::ostringstream s;
    s << "0x" << std::hex << std::setfill('0') << std::setw(digits) << v;
    return s.str();
}

// ISA text with the operands substituted. `imm8`/`w16` are only looked at for
// the opcodes that have them.
static std::string mnemonic(Op op, Byte imm8, std::uint16_t w16) {
    switch (op) {
    case Op::Halt:
        return "HALT";
    case Op::Nop:
        return "NOP";
    case Op::Out:
        return "OUT";
    case Op::Outn:
        return "OUTN";
    case Op::Outs:
        return "OUTS " + hex(w16, 4);
    case Op::Cls:
        return "CLS";
    case Op::Plot:
        return "PLOT";
    case Op::Unplot:
        return "UNPLOT";
    case Op::Show:
        return "SHOW";
    case Op::Alloc:
        return "ALLOC";
    case Op::Add:
        return "ADD A, B";
    case Op::Sub:
        return "SUB A, B";
    case Op::And:
        return "AND A, B";
    case Op::Or:
        return "OR A, B";
    case Op::Xor:
        return "XOR A, B";
    case Op::Not:
        return "NOT A";
    case Op::Shl:
        return "SHL A";
    case Op::Shr:
        return "SHR A";
    case Op::Inc:
        return "INC A";
    case Op::Dec:
        return "DEC A";
    case Op::Cmp:
        return "CMP A, B";
    case Op::LoadiA:
        return "LOADI A, " + hex(imm8, 2);
    case Op::LoadiB:
        return "LOADI B, " + hex(imm8, 2);
    case Op::LoadA:
        return "LOAD A, [" + hex(w16, 4) + "]";
    case Op::LoadB:
        return "LOAD B, [" + hex(w16, 4) + "]";
    case Op::StoreA:
        return "STORE [" + hex(w16, 4) + "], A";
    case Op::StoreB:
        return "STORE [" + hex(w16, 4) + "], B";
    case Op::MovAB:
        return "MOV A, B";
    case Op::MovBA:
        return "MOV B, A";
    case Op::LoadH:
        return "LOADH H, " + hex(w16, 4);
    case Op::LoadAH:
        return "LOAD A, [H]";
    case Op::StoreHA:
        return "STORE [H], A";
    case Op::IncH:
        return "INCH";
    case Op::DecH:
        return "DECH";
    case Op::HLow:
        return "HLOW";
    case Op::Jmp:
        return "JMP " + hex(w16, 4);
    case Op::Jz:
        return "JZ " + hex(w16, 4);
    case Op::Jnz:
        return "JNZ " + hex(w16, 4);
    case Op::Jc:
        return "JC " + hex(w16, 4);
    case Op::Jnc:
        return "JNC " + hex(w16, 4);
    case Op::Jg:
        return "JG " + hex(w16, 4);
    case Op::Jl:
        return "JL " + hex(w16, 4);
    case Op::Free:
        return "FREE";
    }
    return "?";
}

// Read the whole instruction at `at` (PC, for step) before executing it: a
// STORE may overwrite its own bytes. The byte becomes an Op here, once, after
// instr_size has said it is a row of the ISA. 16-bit operands go through the
// given get16.
static StepResult fetch(const Memory& mem, std::size_t at, Instruction& ins) {
    if (at >= MEM_SIZE)
        return StepResult::out_of_memory;
    const Byte op = mem_get(mem, at);
    const std::uint16_t size = instr_size(op);
    if (size == 0)
        return StepResult::bad_opcode;
    // The one bounds check on the instruction stream: never read an operand
    // past the last byte of the box.
    if (at + size > MEM_SIZE)
        return StepResult::out_of_memory;

    ins.at = static_cast<std::uint16_t>(at);
    ins.op = static_cast<Op>(op);
    ins.size = static_cast<Byte>(size);
    ins.bytes[0] = op;
    ins.bytes[1] = size > 1 ? mem_get(mem, at + 1) : Byte{0};
    ins.bytes[2] = size > 2 ? mem_get(mem, at + 2) : Byte{0};
    ins.w16 = size == 3 ? get16(mem, at + 1) : 0;
    return StepResult::ok;
}

std::string disasm(const Memory& mem, std::uint16_t at) {
    Instruction ins{};
    if (fetch(mem, at, ins) != StepResult::ok)
        return "?";
    return mnemonic(ins.op, ins.bytes[1], ins.w16);
}

std::uint16_t data_address(const CPU& cpu) {
    Instruction ins{};
    fetch(*cpu.mem, cpu.pc, ins);
    if (ins.op == Op::LoadAH || ins.op == Op::StoreHA)
        return cpu.h;
    return ins.w16;
}

// Every access the GUEST makes to a byte it names by address goes through
// these two (LOAD/STORE by addr16 and by H, OUTS, the heap's free list). A
// guest address outside the box is refused, never wrapped. The output port
// and the timer live here, in one place, so they work for every way a guest
// reaches them; the host (set, get, plot, sort) uses mem_get/mem_set and sees
// plain memory there. CLS/PLOT/UNPLOT write VRAM through the same plot/clear
// as the REPL: VRAM never overlaps the port.
bool guest_read(const CPU& cpu, std::size_t addr, Byte& out) {
    if (addr >= MEM_SIZE)
        return false;
    if (addr == TICKS)
        // ticks does not count this instruction yet: it is counted when it ends.
        out = static_cast<Byte>(cpu.ticks + 1);
    else
        out = mem_get(*cpu.mem, addr);
    return true;
}

bool guest_write(CPU& cpu, std::size_t addr, Byte value) {
    if (!mem_set(*cpu.mem, addr, value))
        return false; // nothing written: addr >= MEM_SIZE
    if (addr == PORT_OUT)
        guest_putc(value); // every write prints, even the same byte twice
    return true;
}

// A taken jump: the next instruction is at `target`, not after this one (no
// "+ size"). A target outside the box is refused like a bad LOAD/STORE
// address; an untaken jump never looks at it.
static bool jump_to(std::uint16_t target, std::uint16_t& next) {
    if (target >= MEM_SIZE)
        return false;
    next = target;
    return true;
}

// A byte read as two's complement, -128..127, for JG/JL. Spelled out because
// static_cast<std::int8_t>(200) is implementation-defined before C++20.
static int as_signed(Byte v) { return v < 128 ? v : v - 256; }

static void print_flags(const Flags& f) {
    std::cout << "Z=" << f.z << " N=" << f.n << " C=" << f.c;
}

// AAAA  OP B1 B2  MNEMONIC          -> A=nnn  B=nnn  H=0xhhhh  Z=z N=n C=c
static void print_trace(const CPU& cpu, const Instruction& ins, const std::string& text) {
    guest_end_line(); // OUT may have left the line open
    std::cout << std::hex << std::setfill('0') << std::setw(4) << ins.at << ' ';
    for (std::uint16_t i = 0; i < 3; ++i) {
        if (i < ins.size)
            std::cout << ' ' << std::setw(2) << static_cast<int>(ins.bytes[i]);
        else
            std::cout << "   ";
    }
    std::cout << std::dec << std::setfill(' ') << "  " << std::left << std::setw(18) << text
              << std::right << "-> A=" << std::setw(3) << static_cast<int>(cpu.a)
              << "  B=" << std::setw(3) << static_cast<int>(cpu.b) << "  H=0x" << std::hex
              << std::setfill('0') << std::setw(4) << cpu.h << std::dec << std::setfill(' ')
              << "  ";
    print_flags(cpu.f);
    std::cout << '\n';
}

StepResult step(CPU& cpu, bool trace) {
    if (cpu.halted)
        return StepResult::halted;

    Instruction ins{};
    const StepResult fetched = fetch(*cpu.mem, cpu.pc, ins);
    if (fetched != StepResult::ok)
        return fetched;
    const std::uint16_t w16 = ins.w16;
    const Byte imm8 = ins.bytes[1];
    const Decoded d = decode(ins.bytes[0]);
    Byte v = 0;
    std::uint16_t block = 0; // ALLOC: where the new block starts
    bool flag = false;       // ALLOC: the block was reused; FREE: it was the top one
    HeapResult hr = HeapResult::ok;
    // Where PC goes after this instruction: the next one, unless a jump is taken.
    std::uint16_t next = static_cast<std::uint16_t>(ins.at + ins.size);

    // Every error path returns before anything is changed. Each known opcode
    // has its own case; every default is the second place an unknown opcode
    // is caught (after instr_size). The 0x0_, 0x2_, 0x3_ rows and JG/JL never
    // touch the flags (ISA §3); the 0x4_, 0x6_ rows and FREE touch only C.
    switch (d.group) {
    case 0x0:
        switch (ins.op) {
        case Op::Halt:
            cpu.halted = true;
            break;
        case Op::Nop:
            break;
        case Op::Out:
            guest_putc(cpu.a);
            break;
        case Op::Outn:
            guest_putn(cpu.a);
            break;
        case Op::Outs:
            // A C string of the guest: bytes up to the 0 (not printed), but at
            // most OUTS_MAX of them, and never past the box: the end of the box
            // is one more ceiling, not an error. Only a start outside the box
            // is refused, before anything is printed.
            if (w16 >= MEM_SIZE)
                return StepResult::bad_address;
            for (std::size_t addr = w16, n = 0; n < OUTS_MAX && guest_read(cpu, addr, v) && v != 0;
                 ++addr, ++n)
                guest_putc(v);
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x1:
        switch (ins.op) {
        case Op::Add:
            cpu.a = alu_add(cpu.a, cpu.b, cpu.f);
            break;
        case Op::Sub:
            cpu.a = alu_sub(cpu.a, cpu.b, cpu.f);
            break;
        case Op::And:
            cpu.a = alu_and(cpu.a, cpu.b, cpu.f);
            break;
        case Op::Or:
            cpu.a = alu_or(cpu.a, cpu.b, cpu.f);
            break;
        case Op::Xor:
            cpu.a = alu_xor(cpu.a, cpu.b, cpu.f);
            break;
        case Op::Not:
            cpu.a = alu_not(cpu.a, cpu.f);
            break;
        case Op::Shl:
            cpu.a = alu_shl(cpu.a, cpu.f);
            break;
        case Op::Shr:
            cpu.a = alu_shr(cpu.a, cpu.f);
            break;
        case Op::Inc:
            cpu.a = alu_inc(cpu.a, cpu.f);
            break;
        case Op::Dec:
            cpu.a = alu_dec(cpu.a, cpu.f);
            break;
        case Op::Cmp:
            alu_sub(cpu.a, cpu.b, cpu.f); // only the flags; A keeps its value
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x2:
        switch (ins.op) {
        case Op::LoadiA:
            cpu.a = imm8;
            break;
        case Op::LoadiB:
            cpu.b = imm8;
            break;
        case Op::LoadA:
            if (!guest_read(cpu, w16, v))
                return StepResult::bad_address;
            cpu.a = v;
            break;
        case Op::LoadB:
            if (!guest_read(cpu, w16, v))
                return StepResult::bad_address;
            cpu.b = v;
            break;
        case Op::StoreA:
            if (!guest_write(cpu, w16, cpu.a))
                return StepResult::bad_address;
            break;
        case Op::StoreB:
            if (!guest_write(cpu, w16, cpu.b))
                return StepResult::bad_address;
            break;
        case Op::MovAB:
            cpu.a = cpu.b;
            break;
        case Op::MovBA:
            cpu.b = cpu.a;
            break;
        case Op::LoadH:
            cpu.h = w16;
            break;
        case Op::LoadAH:
            if (!guest_read(cpu, cpu.h, v))
                return StepResult::bad_address;
            cpu.a = v;
            break;
        case Op::StoreHA:
            if (!guest_write(cpu, cpu.h, cpu.a))
                return StepResult::bad_address;
            break;
        case Op::IncH:
            // H is 16 bits: 0xFFFF + 1 wraps to 0, which is defined for an
            // unsigned type. Holding a bad address is fine; using it is not.
            cpu.h = static_cast<std::uint16_t>(cpu.h + 1);
            break;
        case Op::DecH:
            cpu.h = static_cast<std::uint16_t>(cpu.h - 1);
            break;
        case Op::HLow:
            cpu.a = static_cast<Byte>(cpu.h & 0xFF);
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x3:
        switch (ins.op) {
        case Op::Jmp:
            if (!jump_to(w16, next))
                return StepResult::bad_address;
            break;
        case Op::Jz:
            if (cpu.f.z && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
        case Op::Jnz:
            if (!cpu.f.z && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
        case Op::Jc:
            if (cpu.f.c && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
        case Op::Jnc:
            if (!cpu.f.c && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x4:
        // The screen. PLOT/UNPLOT touch only C: 1 = off the screen and
        // nothing changed, 0 = done. Z and N are never touched.
        switch (ins.op) {
        case Op::Cls:
            clear(*cpu.mem);
            break;
        case Op::Plot:
            cpu.f.c = !plot(*cpu.mem, cpu.a, cpu.b);
            break;
        case Op::Unplot:
            cpu.f.c = !unplot(*cpu.mem, cpu.a, cpu.b);
            break;
        case Op::Show:
            guest_end_line(); // the screen starts on a line of its own
            show(*cpu.mem);
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x6:
        // The guest heap (heap.cpp). Not fitting is no error of the step:
        // C = 1 and H stays. Z, N, A and B are never touched.
        switch (ins.op) {
        case Op::Alloc:
            hr = heap_alloc(cpu, cpu.a, block, flag);
            if (hr == HeapResult::broken_list)
                return StepResult::bad_heap;
            if (hr == HeapResult::ok)
                cpu.h = block;
            cpu.f.c = hr != HeapResult::ok;
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x7:
        // Own extensions: compare A and B themselves as signed numbers. The
        // flags cannot say this alone: N of A - B is wrong when the
        // subtraction overflows (A = -128, B = 1 gives 0x7F, N = 0).
        switch (ins.op) {
        case Op::Jg:
            if (as_signed(cpu.a) > as_signed(cpu.b) && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
        case Op::Jl:
            if (as_signed(cpu.a) < as_signed(cpu.b) && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
        case Op::Free:
            // Give [H, H + A) back; A is the size asked of ALLOC. A refused
            // block (not in use, already free, ...) is C = 1, not an error.
            // H and A keep their values: H now dangles, as after delete.
            hr = heap_free(cpu, cpu.h, cpu.a, flag);
            if (hr == HeapResult::broken_list)
                return StepResult::bad_heap;
            cpu.f.c = hr != HeapResult::ok;
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    default:
        return StepResult::bad_opcode;
    }

    cpu.pc = next;
    ++cpu.ticks; // a Byte: 255 + 1 wraps to 0, the "mod 256" of TICKS
    mem_set(*cpu.mem, STATUS_ADDR, static_cast<Byte>((cpu.f.z << 2) | (cpu.f.n << 1) | cpu.f.c));

    if (trace)
        print_trace(cpu, ins, mnemonic(ins.op, imm8, w16));
    return StepResult::ok;
}

void dump_regs(const CPU& cpu) {
    std::cout << "PC=0x" << std::hex << std::setfill('0') << std::setw(4) << cpu.pc << std::dec
              << std::setfill(' ') << "  A=" << static_cast<int>(cpu.a)
              << "  B=" << static_cast<int>(cpu.b) << "  H=0x" << std::hex << std::setfill('0')
              << std::setw(4) << cpu.h << std::dec << std::setfill(' ') << "  ";
    print_flags(cpu.f);
    std::cout << '\n';
}
