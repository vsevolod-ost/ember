// cpu.cpp — fetch mem[pc], decode, execute, advance pc by the ISA size.
#include "cpu.hpp"

#include <iomanip>
#include <iostream>

#include "opcodes.hpp"

Decoded decode(Byte op) {
    return {static_cast<Byte>((op >> 4) & 0x0F), static_cast<Byte>(op & 0x0F)};
}

static const char* mnemonic(Op op) {
    switch (op) {
    case Op::Halt:
        return "HALT";
    case Op::Nop:
        return "NOP";
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
    case Op::LoadiA:
        return "LOADI A";
    case Op::LoadiB:
        return "LOADI B";
    }
    return "?";
}

static void print_flags(const Flags& f) {
    std::cout << "Z=" << f.z << " N=" << f.n << " C=" << f.c;
}

StepResult step(CPU& cpu) {
    if (cpu.halted)
        return StepResult::halted;
    if (cpu.pc >= MEM_SIZE)
        return StepResult::out_of_memory;

    const std::uint16_t at = cpu.pc;
    const Byte op = mem_get(*cpu.mem, at);
    const Decoded d = decode(op);
    std::uint16_t size = 1;

    // Every error path returns before anything is changed.
    switch (d.group) {
    case 0x0:
        switch (static_cast<Op>(op)) {
        case Op::Halt:
            cpu.halted = true;
            break;
        case Op::Nop:
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x1:
        switch (static_cast<Op>(op)) {
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
        default:
            return StepResult::bad_opcode;
        }
        break;
    case 0x2:
        switch (static_cast<Op>(op)) {
        case Op::LoadiA:
        case Op::LoadiB:
            size = 2;
            if (at + size > MEM_SIZE)
                return StepResult::out_of_memory;
            // LOADI does not touch the flags (ISA §3).
            (static_cast<Op>(op) == Op::LoadiA ? cpu.a : cpu.b) = mem_get(*cpu.mem, at + 1);
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
    default:
        return StepResult::bad_opcode;
    }

    cpu.pc = static_cast<std::uint16_t>(at + size);
    mem_set(*cpu.mem, STATUS_ADDR, static_cast<Byte>((cpu.f.z << 2) | (cpu.f.n << 1) | cpu.f.c));

    // Trace: where it ran, its bytes, what it is, and the new A and flags.
    std::cout << std::hex << std::setfill('0') << std::setw(4) << at << "  " << std::setw(2)
              << static_cast<int>(op);
    if (size == 2)
        std::cout << ' ' << std::setw(2) << static_cast<int>(mem_get(*cpu.mem, at + 1));
    else
        std::cout << "   ";
    std::cout << std::dec << std::setfill(' ') << "  " << std::left << std::setw(9)
              << mnemonic(static_cast<Op>(op)) << std::right << "-> A=" << std::setw(3)
              << static_cast<int>(cpu.a) << "  ";
    print_flags(cpu.f);
    std::cout << '\n';
    return StepResult::ok;
}

void dump_regs(const CPU& cpu) {
    std::cout << "PC=0x" << std::hex << std::setfill('0') << std::setw(4) << cpu.pc << std::dec
              << std::setfill(' ') << "  A=" << static_cast<int>(cpu.a)
              << "  B=" << static_cast<int>(cpu.b) << "  ";
    print_flags(cpu.f);
    std::cout << '\n';
}
