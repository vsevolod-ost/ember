// expr.hpp — operand expressions (Lab 8 Advanced): `2+3*4`, `[ball+DX]`,
// `0x100-WIDTH`, parsed into a binary tree and evaluated once the labels
// are known.
//
//   expr  := term { ('+' | '-') term }
//   term  := unary { '*' unary }
//   unary := '-' unary | atom
//   atom  := number | char | label | '(' expr ')'
//
// One function per rule, each calling the next (recursive descent): `*`
// binds tighter than `+` and `-` because term is deeper than expr.
#pragma once

#include <string>

#include "label.hpp"
#include "token.hpp"

struct Expr {
    enum class Kind { Num, Name, Add, Sub, Mul, Neg };
    Kind kind;
    long value;       // Num: its value
    std::string name; // Num: the text as written (0x41, '-'); Name: the label
    int line;
    Expr* left;  // Add, Sub, Mul: the left operand; Neg: the only one
    Expr* right; // Add, Sub, Mul: the right operand; otherwise nullptr
};

// Read one expression from `t` on. On success `t` is the first token after
// it (a Comma, RBracket, Newline...). On a syntax error, or past 256
// operands, minus signs and parentheses (so no walk of the tree can run the
// host's stack out): nullptr, `error` is "line N: ...", nothing to free.
Expr* parse_expr(const Token*& t, std::string& error);

enum class EvalResult {
    ok,
    unknown_label, // a Name is not (yet) in `labels`; `error` names it
    error,         // the value went beyond +-0xFFFFFF; `error` says so
};

// The value of the tree, with labels looked up in `labels`.
EvalResult expr_eval(const Expr* e, const Label* labels, long& out, std::string& error);

// The tree, one node per line, two spaces deeper per level.
void expr_print(const Expr* e);

// The tree as text again, with parentheses only where they are needed.
std::string expr_text(const Expr* e);

void expr_destroy(Expr* e);

// Words that are not labels: registers (A, B, H), mnemonics, and the ISA's
// placeholders. "" if `word` is free, else why not: "A is a register".
std::string reserved_word(const std::string& word);
