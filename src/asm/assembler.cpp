// assembler.cpp — the passes over the token list, line by line.
#include "assembler.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "cpu.hpp"
#include "isa.hpp"
#include "lexer.hpp"
#include "macro.hpp"

namespace {

// Which walk over the lines this is.
enum class Pass {
    sizes, // two-pass, pass 1: labels and sizes; operands are parsed, not evaluated
    bytes, // two-pass, pass 2: every label known; write the bytes
    single // one_pass: labels as they come; unknown ones go into the queue
};

struct Asm {
    Program& p;
    Pass pass;
    std::size_t pc;    // where the next byte goes (std::size_t: may pass 0xFFF)
    Token* rows[256];  // each ISA row's text, lexed: the shape of its line
    Fixup* queue_head; // single pass: operands waiting for a label
    Fixup* queue_tail; // push_back here, pop_front at queue_head
    std::string& error;
};

bool fail(Asm& a, int line, const std::string& msg) {
    a.error = "line " + std::to_string(line) + ": " + msg;
    return false;
}

bool at_end(const Token* t) {
    return t->kind == Token::Kind::Newline || t->kind == Token::Kind::Eof;
}

const char* slot_name(Slot s) {
    switch (s) {
    case Slot::imm8:
        return "imm8";
    case Slot::imm16:
        return "imm16";
    case Slot::addr16:
        return "addr16";
    case Slot::byte:
        return "a .byte";
    case Slot::word:
        return "a .word";
    }
    return "?";
}

std::uint16_t slot_size(Slot s) { return s == Slot::imm8 || s == Slot::byte ? 1 : 2; }

Slot slot_of(Operand o) {
    return o == Operand::imm8 ? Slot::imm8 : o == Operand::imm16 ? Slot::imm16 : Slot::addr16;
}

// Does `v` fit the slot? A negative imm8/imm16 is stored as two's
// complement (-1 -> ff); an address must be inside the box.
bool fits(Asm& a, Slot s, long v, int line) {
    if (s == Slot::addr16) {
        if (v >= 0 && static_cast<std::size_t>(v) < MEM_SIZE)
            return true;
        const std::string shown = v < 0 ? std::to_string(v) : hex(static_cast<unsigned>(v), 4);
        return fail(a, line, "address " + shown + " is outside 0.." + std::to_string(MEM_SIZE - 1));
    }
    const long lo = slot_size(s) == 1 ? -128 : -32768, hi = slot_size(s) == 1 ? 255 : 65535;
    if (v >= lo && v <= hi)
        return true;
    return fail(a, line,
                std::to_string(v) + " does not fit in " + slot_name(s) + " (" + std::to_string(lo) +
                    ".." + std::to_string(hi) + ")");
}

// Write one byte of the program at a.pc. Two lines may not write one byte.
bool put(Asm& a, Byte b, int line) {
    if (a.pc >= MEM_SIZE)
        return fail(a, line, "the program does not fit below " + hex(MEM_SIZE, 4));
    if (a.p.owner[a.pc] != 0)
        return fail(a, line,
                    hex(static_cast<unsigned>(a.pc), 4) + " is already used by line " +
                        std::to_string(a.p.owner[a.pc]));
    a.p.bytes[a.pc] = b;
    a.p.owner[a.pc] = line;
    ++a.pc;
    return true;
}

// The bytes of value `v` in slot `s` at a.pc: one byte, or two low first.
bool put_value(Asm& a, Slot s, long v, int line) {
    const auto u = static_cast<unsigned long>(v); // two's complement for negatives
    if (!put(a, static_cast<Byte>(u & 0xFF), line))
        return false;
    return slot_size(s) == 1 || put(a, static_cast<Byte>((u >> 8) & 0xFF), line);
}

// An operand in slot `s`. Pass 1 only reserves its bytes; pass 2 writes its
// value; the single pass writes it, or 00 and a queued Fixup if a label in
// it comes later. Takes `e` (frees it or hands it to the queue).
bool operand(Asm& a, Slot s, Expr* e, int line) {
    if (a.pass == Pass::sizes) {
        expr_destroy(e);
        if (a.pc + slot_size(s) > MEM_SIZE)
            return fail(a, line, "the program does not fit below " + hex(MEM_SIZE, 4));
        a.pc += slot_size(s);
        return true;
    }
    long v = 0;
    const EvalResult r = expr_eval(e, a.p.labels, v, a.error);
    if (r == EvalResult::unknown_label && a.pass == Pass::single) {
        Fixup* f = new Fixup{static_cast<std::uint16_t>(a.pc), s, e, line, "", 0, nullptr};
        if (!a.queue_head)
            a.queue_head = a.queue_tail = f;
        else
            a.queue_tail = a.queue_tail->next = f;
        a.error.clear();                 // not an error yet: the drain decides
        return put_value(a, s, 0, line); // placeholder bytes: 00 (00)
    }
    expr_destroy(e);
    return r == EvalResult::ok && fits(a, s, v, line) && put_value(a, s, v, line);
}

// The shape of a row's line, lexed from its ISA text once per assembly.
bool load_rows(Asm& a) {
    std::size_t count = 0;
    const IsaRow* rows = isa_rows(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::string err;
        a.rows[static_cast<Byte>(rows[i].op)] = lex(rows[i].text, err);
        if (!a.rows[static_cast<Byte>(rows[i].op)])
            return fail(a, 0, "the ISA row " + std::string(rows[i].text) + ": " + err);
    }
    return true;
}

bool is_placeholder(const Token* t) {
    return t->kind == Token::Kind::Ident &&
           (t->text == "imm8" || t->text == "imm16" || t->text == "addr16");
}

// How far the line got along one row before it went wrong.
struct Match {
    int depth;         // tokens of the row matched (the deepest miss is reported)
    std::string error; // why it stopped, when it did
    Expr* expr;        // the operand, on success (if the row has one)
    const Token* end;  // on success: the Newline/Eof of the line
};

// Compare the line from its mnemonic `t` on with the lexed row `shape`:
// the same tokens, but an expression where the row says imm8/imm16/addr16.
Match try_row(const IsaRow& row, const Token* shape, const Token* t) {
    Match m{1, "", nullptr, nullptr};
    std::string prev = t->text; // for "expected X after <prev>"
    const std::string form = " (" + std::string(row.text) + ")";
    const Token* y = t->next;
    for (const Token* x = shape->next; !at_end(x); x = x->next, ++m.depth) {
        if (is_placeholder(x)) {
            if (at_end(y)) {
                m.error = "expected " + x->text + " after " + prev + form;
                break;
            }
            Expr* e = parse_expr(y, m.error);
            if (!e) {
                // parse_expr wrote "line N: ..."; keep just the reason
                m.error = m.error.substr(m.error.find(": ") + 2);
                break;
            }
            prev = expr_text(e);
            m.expr = e;
            continue;
        }
        if (y->kind != x->kind || y->text != x->text) {
            m.error = "expected " + x->text + " after " + prev + form;
            break;
        }
        prev = y->text;
        y = y->next;
    }
    if (m.error.empty() && !at_end(y))
        m.error = "unexpected " + token_word(y) + " after " + prev;
    if (!m.error.empty()) {
        expr_destroy(m.expr);
        m.expr = nullptr;
        return m;
    }
    m.end = y;
    return m;
}

// The instruction at `t` (an Ident): find its row, emit opcode and operand.
bool instruction(Asm& a, const Token*& t) {
    const int line = t->line;
    if (!is_mnemonic(t->text)) {
        std::string upper = t->text;
        for (char& c : upper)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (is_mnemonic(upper))
            return fail(a, line,
                        "unknown mnemonic " + t->text +
                            " (mnemonics are written in capitals, as in the ISA)");
        return fail(a, line, "unknown mnemonic " + t->text);
    }
    std::size_t count = 0;
    const IsaRow* rows = isa_rows(count);
    Match best{-1, "", nullptr, nullptr};
    // Rows without a placeholder first: LOAD A, [H] before LOAD A, [addr16].
    for (int with_operand = 0; with_operand < 2; ++with_operand) {
        for (std::size_t i = 0; i < count; ++i) {
            const IsaRow& row = rows[i];
            if ((row_operand(row) != Operand::none) != (with_operand == 1))
                continue;
            const Token* shape = a.rows[static_cast<Byte>(row.op)];
            if (shape->text != t->text)
                continue;
            Match m = try_row(row, shape, t);
            if (m.error.empty()) {
                const std::uint16_t size = row_size(row);
                if (a.pass == Pass::sizes && a.pc + size > MEM_SIZE) {
                    expr_destroy(m.expr);
                    return fail(a, line, "the program does not fit below " + hex(MEM_SIZE, 4));
                }
                t = m.end;
                if (a.pass == Pass::sizes) {
                    a.pc += 1;
                } else if (!put(a, static_cast<Byte>(row.op), line)) {
                    expr_destroy(m.expr);
                    return false;
                }
                if (row_operand(row) == Operand::none)
                    return true;
                return operand(a, slot_of(row_operand(row)), m.expr, line);
            }
            if (m.depth > best.depth) {
                best.depth = m.depth;
                best.error = m.error;
            }
        }
    }
    return fail(a, line, best.error);
}

// One expression item of .byte/.word at `t`; `prev` becomes its text.
bool data_item(Asm& a, Slot s, const Token*& t, std::string& prev) {
    const int line = t->line;
    Expr* e = parse_expr(t, a.error);
    if (!e)
        return false;
    prev = expr_text(e);
    return operand(a, s, e, line);
}

// After an item: a comma and another item (true), or the end of the line
// (false, `done`). Anything else is an error (false, not `done`).
bool more_items(Asm& a, const Token*& t, const std::string& prev, bool& done) {
    done = at_end(t);
    if (done)
        return false;
    if (t->kind != Token::Kind::Comma)
        return fail(a, t->line, "unexpected " + token_word(t) + " after " + prev);
    t = t->next;
    if (at_end(t))
        return fail(a, t->line, "expected a value after ,");
    return true;
}

// The bytes of a string, from a.pc on (sizes: just counted).
bool put_string(Asm& a, const std::string& bytes, int line) {
    if (a.pass == Pass::sizes) {
        if (a.pc + bytes.size() > MEM_SIZE)
            return fail(a, line, "the program does not fit below " + hex(MEM_SIZE, 4));
        a.pc += bytes.size();
        return true;
    }
    for (char c : bytes)
        if (!put(a, static_cast<Byte>(c), line))
            return false;
    return true;
}

bool directive(Asm& a, const Token*& t) {
    const Token* d = t;
    const int line = d->line;
    std::string prev = d->text; // for "unexpected X after <prev>"
    t = t->next;
    if (d->text == ".org") {
        if (at_end(t))
            return fail(a, line, "expected an address after .org");
        Expr* e = parse_expr(t, a.error);
        if (!e)
            return false;
        // Labels further down have no address in pass 1 yet, so .org may only
        // use what is known above it: then both passes agree.
        long v = 0;
        const EvalResult r = expr_eval(e, a.p.labels, v, a.error);
        prev = expr_text(e);
        expr_destroy(e);
        if (r != EvalResult::ok)
            return false;
        if (v < 0 || static_cast<std::size_t>(v) >= MEM_SIZE)
            return fail(a, line,
                        ".org " + (v < 0 ? std::to_string(v) : hex(static_cast<unsigned>(v), 4)) +
                            " is outside 0.." + std::to_string(MEM_SIZE - 1));
        a.pc = static_cast<std::size_t>(v);
    } else if (d->text == ".byte" || d->text == ".word") {
        const bool bytes = d->text == ".byte";
        if (at_end(t))
            return fail(a, line, "expected a value after " + d->text);
        bool done = false;
        do {
            if (bytes && t->kind == Token::Kind::String) {
                // A string in .byte: its bytes, without the 0 of .string.
                prev = t->text;
                if (!put_string(a, string_bytes(t->text), line))
                    return false;
                t = t->next;
            } else if (!data_item(a, bytes ? Slot::byte : Slot::word, t, prev)) {
                return false;
            }
        } while (more_items(a, t, prev, done));
        if (!done)
            return false;
    } else if (d->text == ".string") {
        if (t->kind != Token::Kind::String)
            return fail(a, line, "expected a \"string\" after .string, got " + token_word(t));
        std::string bytes = string_bytes(t->text);
        bytes += '\0'; // a C string: OUTS stops at the 0
        if (!put_string(a, bytes, line))
            return false;
        prev = t->text;
        t = t->next;
    } else {
        return fail(a, line, "unknown directive " + d->text);
    }
    if (!at_end(t))
        return fail(a, line, "unexpected " + token_word(t) + " after " + prev);
    return true;
}

void place(Program& p, int line, std::size_t at, std::size_t count) {
    Placed* r = new Placed{line, static_cast<std::uint16_t>(at), static_cast<std::uint16_t>(count),
                           nullptr};
    if (!p.placed)
        p.placed = p.placed_tail = r;
    else
        p.placed_tail = p.placed_tail->next = r;
}

// One source line from `t` on; `t` ends on the first token of the next line.
bool assemble_line(Asm& a, const Token*& t) {
    const int line = t->line;
    bool labelled = false;
    while (t->kind == Token::Kind::Ident && t->next->kind == Token::Kind::Colon) {
        const std::string why = reserved_word(t->text);
        if (!why.empty())
            return fail(a, line, why + ", not a label");
        if (a.pass != Pass::bytes) {
            if (const Label* old = label_find(a.p.labels, t->text))
                return fail(a, line,
                            "label " + t->text + " is already defined on line " +
                                std::to_string(old->line));
            label_add(a.p.labels, t->text, static_cast<std::uint16_t>(a.pc), line);
        }
        labelled = true;
        t = t->next->next;
    }
    const std::size_t start = a.pc;
    bool org = false;
    if (t->kind == Token::Kind::Directive) {
        org = t->text == ".org";
        if (!directive(a, t))
            return false;
    } else if (t->kind == Token::Kind::Ident) {
        if (!instruction(a, t))
            return false;
    } else if (!at_end(t)) {
        return fail(a, line,
                    "expected a label, an instruction or a directive, got " + token_word(t));
    }
    if (a.pass != Pass::sizes && (labelled || (!org && a.pc > start)))
        place(a.p, line, start, org ? 0 : a.pc - start);
    if (t->kind == Token::Kind::Newline)
        t = t->next;
    return true;
}

bool walk(Asm& a, const Token* tokens) {
    a.pc = 0;
    for (const Token* t = tokens; t->kind != Token::Kind::Eof;)
        if (!assemble_line(a, t))
            return false;
    return true;
}

// Single pass: every label is known now. Patch the queued operands in the
// order they were queued (pop_front), keeping each one for the listing.
bool drain(Asm& a) {
    while (a.queue_head) {
        Fixup* f = a.queue_head;
        a.queue_head = f->next;
        f->next = nullptr;
        long v = 0;
        const EvalResult r = expr_eval(f->expr, a.p.labels, v, a.error);
        f->text = expr_text(f->expr);
        expr_destroy(f->expr);
        f->expr = nullptr;
        f->value = v;
        Program& p = a.p;
        if (!p.patched)
            p.patched = p.patched_tail = f;
        else
            p.patched_tail = p.patched_tail->next = f;
        if (r != EvalResult::ok || !fits(a, f->slot, v, f->line))
            return false;
        const auto u = static_cast<unsigned long>(v);
        p.bytes[f->at] = static_cast<Byte>(u & 0xFF);
        if (slot_size(f->slot) == 2)
            p.bytes[f->at + 1] = static_cast<Byte>((u >> 8) & 0xFF);
    }
    return true;
}

void queue_destroy(Fixup* f) {
    while (f) {
        Fixup* next = f->next;
        expr_destroy(f->expr);
        delete f;
        f = next;
    }
}

} // namespace

bool assemble(const std::string& src, AsmMode mode, Program& p, std::string& error) {
    Token* tokens = lex(src, error);
    if (!tokens)
        return false;
    if (!expand_defines(tokens, error)) {
        destroy(tokens);
        return false;
    }
    Asm a{p, Pass::sizes, 0, {}, nullptr, nullptr, error};
    bool ok = load_rows(a);
    if (ok && mode == AsmMode::two_pass) {
        ok = walk(a, tokens);
        a.pass = Pass::bytes;
        ok = ok && walk(a, tokens);
    } else if (ok) {
        a.pass = Pass::single;
        ok = walk(a, tokens) && drain(a);
    }
    queue_destroy(a.queue_head);
    for (Token* row : a.rows)
        destroy(row);
    destroy(tokens);
    if (!ok)
        program_free(p);
    return ok;
}

// `s` without the spaces (and a '\r') at its end.
static std::string rstrip(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r'))
        s.pop_back();
    return s;
}

std::string listing_line(std::uint16_t at, const Byte* bytes, std::uint16_t n,
                         const std::string& text) {
    std::ostringstream line;
    line << std::hex << std::setfill('0') << std::setw(4) << at << ' ';
    for (std::uint16_t i = 0; i < 3; ++i) {
        if (i < n)
            line << ' ' << std::setw(2) << static_cast<int>(bytes[i]);
        else
            line << "   ";
    }
    line << "  " << text;
    return rstrip(line.str());
}

void print_listing(const Program& p, const std::string& src) {
    const Placed* r = p.placed;
    int line = 1;
    for (std::size_t pos = 0; pos < src.size(); ++line) {
        std::size_t nl = src.find('\n', pos);
        if (nl == std::string::npos)
            nl = src.size();
        const std::string text = rstrip(src.substr(pos, nl - pos));
        pos = nl + 1;
        if (!r || r->line != line) {
            std::cout << rstrip(std::string(16, ' ') + text) << '\n';
            continue;
        }
        // AAAA  BB BB BB  text; more than 3 bytes go on below, 3 per line.
        std::cout << listing_line(r->at, &p.bytes[r->at], r->count < 3 ? r->count : 3, text)
                  << '\n';
        for (std::uint16_t done = 3; done < r->count; done += 3) {
            const auto n = static_cast<std::uint16_t>(r->count - done < 3 ? r->count - done : 3);
            const auto at = static_cast<std::uint16_t>(r->at + done);
            std::cout << listing_line(at, &p.bytes[at], n, "") << '\n';
        }
        r = r->next;
    }

    std::cout << "labels:";
    if (!p.labels)
        std::cout << " none";
    for (const Label* l = p.labels; l; l = l->next)
        std::cout << ' ' << l->name << '=' << hex(l->addr, 4);
    std::cout << '\n';

    for (const Fixup* f = p.patched; f; f = f->next) {
        const auto u = static_cast<unsigned long>(f->value);
        std::cout << "fixup " << hex(f->at, 4) << " <- " << f->text << " = "
                  << (slot_size(f->slot) == 1 ? hex(u & 0xFF, 2) : hex(u & 0xFFFF, 4)) << " (line "
                  << f->line << ")\n";
    }

    // Where the bytes went, as runs of written addresses.
    std::size_t total = 0;
    std::string runs;
    for (std::size_t at = 0; at < MEM_SIZE; ++at) {
        if (p.owner[at] == 0)
            continue;
        std::size_t end = at;
        while (end + 1 < MEM_SIZE && p.owner[end + 1] != 0)
            ++end;
        runs += (runs.empty() ? ": " : ", ") + hex(static_cast<unsigned>(at), 4);
        if (end > at)
            runs += "-" + hex(static_cast<unsigned>(end), 4);
        total += end - at + 1;
        at = end;
    }
    std::cout << "assembled " << total << (total == 1 ? " byte" : " bytes") << runs
              << "; PC=" << hex(0, 4) << '\n';
}

void load_program(const Program& p, Memory& mem) {
    for (std::size_t at = 0; at < MEM_SIZE; ++at)
        if (p.owner[at] != 0)
            mem_set(mem, at, p.bytes[at]);
}

void program_free(Program& p) {
    labels_destroy(p.labels);
    p.labels = nullptr;
    while (p.placed) {
        Placed* next = p.placed->next;
        delete p.placed;
        p.placed = next;
    }
    p.placed_tail = nullptr;
    queue_destroy(p.patched);
    p.patched = p.patched_tail = nullptr;
}
