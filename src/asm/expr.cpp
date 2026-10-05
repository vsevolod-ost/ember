// expr.cpp — recursive descent over the token list into a binary tree, and
// back out of it: evaluate, print, free.
#include "expr.hpp"

#include <iostream>

#include "isa.hpp"

// The biggest value a node may have. Two such values multiplied still fit
// in a long long, so no step of eval can overflow before it is checked.
static const long long VALUE_MAX = 0xFFFFFF;

std::string reserved_word(const std::string& word) {
    if (word == "A" || word == "B" || word == "H")
        return word + " is a register";
    if (is_mnemonic(word))
        return word + " is a mnemonic";
    if (word == "imm8" || word == "imm16" || word == "addr16")
        return word + " is a placeholder of the ISA table";
    return "";
}

static Expr* node(Expr::Kind kind, int line, Expr* left, Expr* right) {
    return new Expr{kind, 0, "", line, left, right};
}

static std::string at_line(int line) { return "line " + std::to_string(line) + ": "; }

// At most this many operands, minus signs and opening parentheses in one
// expression. The parser and every walk of the tree (eval, print, text,
// destroy) go one call deeper per level, and each level costs at least one
// of these, so the cap keeps a pathological line from running the host's
// stack out: it is an error with a line number instead.
static const int PARTS_MAX = 256;

static Expr* expr(const Token*& t, std::string& error, int& parts);

static Expr* atom(const Token*& t, std::string& error, int& parts) {
    const Token* tok = t;
    switch (tok->kind) {
    case Token::Kind::Number: {
        long v = 0;
        if (!number_value(tok->text, v)) {
            error = at_line(tok->line) + "number " + tok->text + " is too big";
            return nullptr;
        }
        t = tok->next;
        return new Expr{Expr::Kind::Num, v, tok->text, tok->line, nullptr, nullptr};
    }
    case Token::Kind::Char:
        t = tok->next;
        return new Expr{Expr::Kind::Num, char_value(tok->text), tok->text, tok->line, nullptr,
                        nullptr};
    case Token::Kind::Ident: {
        const std::string why = reserved_word(tok->text);
        if (!why.empty()) {
            error = at_line(tok->line) + why + ", not a label";
            return nullptr;
        }
        t = tok->next;
        return new Expr{Expr::Kind::Name, 0, tok->text, tok->line, nullptr, nullptr};
    }
    case Token::Kind::LParen: {
        t = tok->next;
        Expr* inside = expr(t, error, parts);
        if (!inside)
            return nullptr;
        if (t->kind != Token::Kind::RParen) {
            error = at_line(t->line) + "expected ) before " + token_word(t);
            expr_destroy(inside);
            return nullptr;
        }
        t = t->next;
        return inside;
    }
    default:
        error = at_line(tok->line) + "expected a number or a label, got " + token_word(tok);
        return nullptr;
    }
}

// Every operand, minus sign and `(` passes through here once: the count.
static Expr* unary(const Token*& t, std::string& error, int& parts) {
    if (++parts > PARTS_MAX) {
        error = at_line(t->line) + "expression has more than " + std::to_string(PARTS_MAX) +
                " operands, minus signs and parentheses";
        return nullptr;
    }
    if (t->kind != Token::Kind::Minus)
        return atom(t, error, parts);
    const int line = t->line;
    t = t->next;
    Expr* operand = unary(t, error, parts);
    return operand ? node(Expr::Kind::Neg, line, operand, nullptr) : nullptr;
}

static Expr* term(const Token*& t, std::string& error, int& parts) {
    Expr* left = unary(t, error, parts);
    while (left && t->kind == Token::Kind::Star) {
        const int line = t->line;
        t = t->next;
        Expr* right = unary(t, error, parts);
        if (!right) {
            expr_destroy(left);
            return nullptr;
        }
        left = node(Expr::Kind::Mul, line, left, right); // left-associative
    }
    return left;
}

static Expr* expr(const Token*& t, std::string& error, int& parts) {
    Expr* left = term(t, error, parts);
    while (left && (t->kind == Token::Kind::Plus || t->kind == Token::Kind::Minus)) {
        const Expr::Kind kind = t->kind == Token::Kind::Plus ? Expr::Kind::Add : Expr::Kind::Sub;
        const int line = t->line;
        t = t->next;
        Expr* right = term(t, error, parts);
        if (!right) {
            expr_destroy(left);
            return nullptr;
        }
        left = node(kind, line, left, right);
    }
    return left;
}

Expr* parse_expr(const Token*& t, std::string& error) {
    int parts = 0;
    return expr(t, error, parts);
}

static EvalResult eval(const Expr* e, const Label* labels, long long& out, std::string& error) {
    long long l = 0, r = 0;
    EvalResult res = EvalResult::ok;
    switch (e->kind) {
    case Expr::Kind::Num:
        out = e->value;
        return EvalResult::ok;
    case Expr::Kind::Name: {
        const Label* label = label_find(labels, e->name);
        if (!label) {
            error = at_line(e->line) + "unknown label " + e->name;
            return EvalResult::unknown_label;
        }
        out = label->addr;
        return EvalResult::ok;
    }
    case Expr::Kind::Neg:
        res = eval(e->left, labels, l, error);
        out = -l;
        return res;
    case Expr::Kind::Add:
    case Expr::Kind::Sub:
    case Expr::Kind::Mul:
        break;
    }
    if ((res = eval(e->left, labels, l, error)) != EvalResult::ok)
        return res;
    if ((res = eval(e->right, labels, r, error)) != EvalResult::ok)
        return res;
    out = e->kind == Expr::Kind::Add ? l + r : e->kind == Expr::Kind::Sub ? l - r : l * r;
    if (out > VALUE_MAX || out < -VALUE_MAX) {
        error = at_line(e->line) + expr_text(e) + " is beyond -0xffffff..0xffffff";
        return EvalResult::error;
    }
    return EvalResult::ok;
}

EvalResult expr_eval(const Expr* e, const Label* labels, long& out, std::string& error) {
    long long v = 0;
    const EvalResult res = eval(e, labels, v, error);
    if (res == EvalResult::ok)
        out = static_cast<long>(v); // within +-0xFFFFFF: fits a long
    return res;
}

static const char* op_text(Expr::Kind kind) {
    switch (kind) {
    case Expr::Kind::Add:
        return "+";
    case Expr::Kind::Sub:
        return "-";
    case Expr::Kind::Mul:
        return "*";
    case Expr::Kind::Neg:
        return "neg";
    case Expr::Kind::Num:
    case Expr::Kind::Name:
        break;
    }
    return "";
}

static void print(const Expr* e, int depth) {
    std::cout << std::string(2 * static_cast<std::size_t>(depth), ' ');
    if (e->kind == Expr::Kind::Num || e->kind == Expr::Kind::Name) {
        std::cout << e->name << '\n';
        return;
    }
    std::cout << op_text(e->kind) << '\n';
    print(e->left, depth + 1);
    if (e->right)
        print(e->right, depth + 1);
}

void expr_print(const Expr* e) { print(e, 0); }

// How tightly a node binds: a child that binds looser than its parent needs
// parentheses to keep its shape.
static int binding(const Expr* e) {
    switch (e->kind) {
    case Expr::Kind::Add:
    case Expr::Kind::Sub:
        return 1;
    case Expr::Kind::Mul:
        return 2;
    case Expr::Kind::Neg:
        return 3;
    case Expr::Kind::Num:
    case Expr::Kind::Name:
        break;
    }
    return 4;
}

static std::string wrapped(const Expr* e, bool parens) {
    return parens ? "(" + expr_text(e) + ")" : expr_text(e);
}

std::string expr_text(const Expr* e) {
    if (e->kind == Expr::Kind::Num || e->kind == Expr::Kind::Name)
        return e->name;
    if (e->kind == Expr::Kind::Neg)
        return "-" + wrapped(e->left, binding(e->left) < 3);
    // The right operand of - and * also needs them at equal binding:
    // a-(b-c) is not a-b-c.
    return wrapped(e->left, binding(e->left) < binding(e)) + op_text(e->kind) +
           wrapped(e->right, binding(e->right) <= binding(e) && e->kind != Expr::Kind::Add);
}

void expr_destroy(Expr* e) {
    if (!e)
        return;
    expr_destroy(e->left);
    expr_destroy(e->right);
    delete e;
}
