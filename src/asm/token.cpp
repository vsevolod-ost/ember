// token.cpp — the token list besides what the given lexer.cpp has (emit and
// destroy): push_back, cut and splice for pass 0, printing, and reading the
// value back out of a literal's text.
#include "token.hpp"

#include <iomanip>
#include <iostream>

void push_back(Token*& head, Token*& tail, Token::Kind kind, const std::string& text, int line) {
    Token* t = new Token{kind, text, line, nullptr};
    if (!head)
        head = tail = t;
    else
        tail = tail->next = t;
}

void cut(Token** link, Token* last) {
    Token* first = *link;
    *link = last->next;
    last->next = nullptr;
    destroy(first);
}

void splice(Token** link, Token* first, Token* last) {
    last->next = *link;
    *link = first;
}

const char* kind_name(Token::Kind kind) {
    switch (kind) {
    case Token::Kind::Ident:
        return "Ident";
    case Token::Kind::Number:
        return "Number";
    case Token::Kind::Comma:
        return "Comma";
    case Token::Kind::Colon:
        return "Colon";
    case Token::Kind::LBracket:
        return "LBracket";
    case Token::Kind::RBracket:
        return "RBracket";
    case Token::Kind::Newline:
        return "Newline";
    case Token::Kind::Eof:
        return "Eof";
    case Token::Kind::Directive:
        return "Directive";
    case Token::Kind::String:
        return "String";
    case Token::Kind::Char:
        return "Char";
    case Token::Kind::Plus:
        return "Plus";
    case Token::Kind::Minus:
        return "Minus";
    case Token::Kind::Star:
        return "Star";
    case Token::Kind::LParen:
        return "LParen";
    case Token::Kind::RParen:
        return "RParen";
    }
    return "?";
}

// Kinds whose text says more than the kind: they print as Kind(text).
static bool has_text(Token::Kind kind) {
    return kind == Token::Kind::Ident || kind == Token::Kind::Number ||
           kind == Token::Kind::Directive || kind == Token::Kind::String ||
           kind == Token::Kind::Char;
}

void print_tokens(const Token* head) {
    std::size_t n = 0;
    for (const Token* t = head; t; t = t->next) {
        std::cout << std::setw(4) << t->line << "  " << kind_name(t->kind);
        if (has_text(t->kind))
            std::cout << '(' << t->text << ')';
        std::cout << '\n';
        ++n;
    }
    std::cout << n << (n == 1 ? " token\n" : " tokens\n");
}

std::string token_word(const Token* t) {
    if (t->kind == Token::Kind::Newline)
        return "end of line";
    if (t->kind == Token::Kind::Eof)
        return "end of file";
    return t->text;
}

// Values the assembler can hold without overflow, even multiplied (expr.cpp).
static const long VALUE_MAX = 0xFFFFFF;

bool number_value(const std::string& text, long& out) {
    int base = 10;
    std::size_t i = 0;
    if (text.size() > 1 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        i = 2;
    } else if (text.size() > 1 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
        base = 2;
        i = 2;
    }
    long v = 0;
    for (; i < text.size(); ++i) {
        const char c = text[i];
        const int digit = c >= '0' && c <= '9'   ? c - '0'
                          : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                 : c - 'A' + 10;
        v = v * base + digit;
        if (v > VALUE_MAX)
            return false; // stop before v could overflow
    }
    out = v;
    return true;
}

// The byte an escape stands for: the character after the backslash.
static unsigned char unescape(char c) {
    switch (c) {
    case 'n':
        return '\n';
    case 't':
        return '\t';
    case '0':
        return 0;
    default:
        return static_cast<unsigned char>(c); // \\ \" \'
    }
}

unsigned char char_value(const std::string& text) {
    // 'c' or '\c': the quotes are text[0] and the last character.
    return text[1] == '\\' ? unescape(text[2]) : static_cast<unsigned char>(text[1]);
}

std::string string_bytes(const std::string& text) {
    std::string bytes;
    for (std::size_t i = 1; i + 1 < text.size(); ++i) {
        if (text[i] == '\\') {
            bytes += static_cast<char>(unescape(text[i + 1]));
            ++i;
        } else {
            bytes += text[i];
        }
    }
    return bytes;
}
