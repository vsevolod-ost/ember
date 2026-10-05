// token.hpp — the token node of Lab 8 theory §3 (GIVEN), plus the destroy()
// the lab declares here. Our kinds (Lab 8 Advanced: directives, strings,
// characters, the operators of expressions) are added AFTER Eof, so the given
// ones keep their values.
#pragma once

#include <string>

struct Token {
    enum class Kind {
        Ident,
        Number,
        Comma,
        Colon,
        LBracket,
        RBracket,
        Newline,
        Eof,
        // Lab 8 Advanced
        Directive, // .org .byte .word .string .define
        String,    // "Hello\n", the text as written, quotes included
        Char,      // '-', the text as written, quotes included
        Plus,
        Minus,
        Star,
        LParen,
        RParen
    };
    Kind kind;
    std::string text; // або зріз (вказівник + довжина) у вихідний текст
    int line;
    Token* next; // nullptr у кінці
};

void destroy(Token* head);

// The rest of the list as an abstract type (Notes 08 §3: push_back, walk,
// destroy). The walk is `for (const Token* t = head; t; t = t->next)`. The
// given lexer has its own push_back (emit, kept as given); everything else
// that builds or edits a token list (pass 0, macro.cpp) uses these.

// A new node {kind, text, line} at the end of [head, tail].
void push_back(Token*& head, Token*& tail, Token::Kind kind, const std::string& text, int line);
// `*link` is the pointer that points at a node: head, or the previous
// node's next. Unlink that node and the ones after it up to and including
// `last`, and free them.
void cut(Token** link, Token* last);
// Put the list [first, last] where `*link` points, before the node that was
// there.
void splice(Token** link, Token* first, Token* last);

// The kind's name as `lex` prints it: "Ident", "Comma", ...
const char* kind_name(Token::Kind kind);

// `lex <file>`: one token per line, "   1  Ident(LOADI)", then "N tokens".
void print_tokens(const Token* head);

// A token as a word for an error message: its text, or "end of line" for a
// Newline and "end of file" for Eof.
std::string token_word(const Token* t);

// The value of a Number token's text (0x41, 0b101, 65). False for a value
// above 0xFFFFFF: the assembler says "too big" instead of wrapping.
bool number_value(const std::string& text, long& out);

// The byte of a Char token's text ('-', '\n'). The lexer has checked it.
unsigned char char_value(const std::string& text);

// The bytes of a String token's text, escapes decoded, quotes dropped, no 0
// at the end. The lexer has checked the escapes.
std::string string_bytes(const std::string& text);
