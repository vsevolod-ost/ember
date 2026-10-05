// macro.cpp — pass 0: .define. Two linked lists meet here: the program's
// tokens, which are cut and spliced in place (token.hpp), and the macros,
// each holding its own copy of its tokens.
#include "macro.hpp"

#include "expr.hpp"

// At most this many token copies in one pass 0 (bodies and uses together).
// A macro may use the one before it twice, so ten lines can ask for 2^10
// times the text: past the cap that is an error, not gigabytes of nodes.
static const long COPIES_MAX = 65536;

struct Macro {
    std::string name;
    Token* body; // its own nodes, freed with the macro
    int line;
    Macro* next;
};

static const Macro* macro_find(const Macro* head, const std::string& name) {
    for (const Macro* m = head; m; m = m->next)
        if (m->name == name)
            return m;
    return nullptr;
}

static void macros_destroy(Macro* head) {
    while (head) {
        Macro* next = head->next;
        destroy(head->body);
        delete head;
        head = next;
    }
}

static bool fail(std::string& error, int line, const std::string& msg) {
    error = "line " + std::to_string(line) + ": " + msg;
    return false;
}

// Append a copy of `t` to [head, tail], on line `line`; `copies` counts it
// against COPIES_MAX. False (and `error`) past the cap.
static bool copy_token(const Token* t, Token*& head, Token*& tail, int line, long& copies,
                       std::string& error) {
    if (++copies > COPIES_MAX)
        return fail(error, line,
                    ".define expands to more than " + std::to_string(COPIES_MAX) + " tokens");
    push_back(head, tail, t->kind, t->text, line);
    return true;
}

// The same for every node of the list `from`.
static bool copy_list(const Token* from, Token*& head, Token*& tail, int line, long& copies,
                      std::string& error) {
    for (const Token* t = from; t; t = t->next)
        if (!copy_token(t, head, tail, line, copies, error))
            return false;
    return true;
}

// The `.define` at `def`: check it and make its macro (body already expanded).
static bool define(const Token* def, Macro*& macros, Macro*& last, long& copies,
                   std::string& error) {
    const Token* name = def->next;
    if (name->kind != Token::Kind::Ident)
        return fail(error, def->line, "expected a name after .define, got " + token_word(name));
    const std::string why = reserved_word(name->text);
    if (!why.empty())
        return fail(error, def->line, why + ", not a .define name");
    if (const Macro* old = macro_find(macros, name->text))
        return fail(error, def->line,
                    name->text + " is already defined on line " + std::to_string(old->line));
    const Token* t = name->next;
    if (t->kind == Token::Kind::Newline || t->kind == Token::Kind::Eof)
        return fail(error, def->line, ".define " + name->text + " has no value");

    Token *body = nullptr, *tail = nullptr;
    for (; t->kind != Token::Kind::Newline && t->kind != Token::Kind::Eof; t = t->next) {
        const Macro* used = t->kind == Token::Kind::Ident ? macro_find(macros, t->text) : nullptr;
        const bool ok = used ? copy_list(used->body, body, tail, def->line, copies, error)
                             : copy_token(t, body, tail, def->line, copies, error);
        if (!ok) {
            destroy(body);
            return false;
        }
    }
    Macro* m = new Macro{name->text, body, def->line, nullptr};
    if (!macros)
        macros = last = m;
    else
        last = last->next = m;
    return true;
}

bool expand_defines(Token*& head, std::string& error) {
    Macro *macros = nullptr, *last = nullptr;
    long copies = 0;
    bool line_start = true;
    // `link` is the pointer that points at the current token (head, or the
    // previous node's next), so a token can be replaced where it stands.
    Token** link = &head;
    while (*link) {
        Token* t = *link;
        if (t->kind == Token::Kind::Directive && t->text == ".define") {
            if (!line_start) {
                macros_destroy(macros);
                return fail(error, t->line, ".define must start its line");
            }
            if (!define(t, macros, last, copies, error)) {
                macros_destroy(macros);
                return false;
            }
            // Cut [.define, Newline) out; the Newline stays in the list.
            Token* end = t;
            while (end->next->kind != Token::Kind::Newline && end->next->kind != Token::Kind::Eof)
                end = end->next;
            cut(link, end);
            continue;
        }
        const Macro* m = t->kind == Token::Kind::Ident ? macro_find(macros, t->text) : nullptr;
        if (m) {
            Token *copy = nullptr, *tail = nullptr;
            if (!copy_list(m->body, copy, tail, t->line, copies, error)) {
                destroy(copy);
                macros_destroy(macros);
                return false;
            }
            cut(link, t);             // the use goes,
            splice(link, copy, tail); // the copy of the body stands where it was
            link = &tail->next;
            line_start = false;
            continue;
        }
        line_start = t->kind == Token::Kind::Newline;
        link = &t->next;
    }
    macros_destroy(macros);
    return true;
}
