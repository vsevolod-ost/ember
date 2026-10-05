// lexer.cpp — ДАНО, крім number(). Прочитайте цикл у lex() згори донизу:
// це псевдокод з теорії §1, записаний на C++.
#include "lexer.hpp"

#include <cctype>

// Стан лексера: текст, курсор, номер рядка і список, який ми будуємо.
struct Lexer {
    const std::string& src;
    std::size_t i = 0;
    int line = 1;
    Token* head = nullptr;
    Token* tail = nullptr;
    std::string error; // непорожній, щойно лексер щось відхилив
};

// Додати вузол у кінець списку (Notes 08 §3).
static void emit(Lexer& lx, Token::Kind kind, const std::string& text) {
    Token* t = new Token{kind, text, lx.line, nullptr};
    if (!lx.head) {
        lx.head = lx.tail = t;
    } else {
        lx.tail->next = t;
        lx.tail = t;
    }
}

// Записати помилку з номером рядка. Повертає false, щоб зручно писати
// `return fail(lx, "...");`
static bool fail(Lexer& lx, const std::string& msg) {
    lx.error = "line " + std::to_string(lx.line) + ": " + msg;
    return false;
}

void destroy(Token* head) {
    while (head) {
        Token* next = head->next;
        delete head;
        head = next;
    }
}

// ВАШЕ (Lab 8, M1). Курсор lx.i стоїть на цифрі.
// З'їсти число — `0x` і шістнадцяткові цифри, `0b` і двійкові, або десяткові —
// і видати його: emit(lx, Token::Kind::Number, текст_числа).
// Зіпсоване число — `0xGG`, `0x` без жодної цифри, `12ab` — це
// `return fail(lx, "bad number");`, а не тихий нуль.
static bool number(Lexer& lx) {
    const std::string& s = lx.src;
    const std::size_t start = lx.i;
    std::size_t digits = 0; // after the 0x/0b prefix, if any
    const char kind = s[lx.i] == '0' && lx.i + 1 < s.size() ? s[lx.i + 1] : '\0';
    if (kind == 'x' || kind == 'X') {
        lx.i += 2;
        for (; lx.i < s.size() && std::isxdigit((unsigned char)s[lx.i]); ++lx.i)
            ++digits;
    } else if (kind == 'b' || kind == 'B') {
        lx.i += 2;
        for (; lx.i < s.size() && (s[lx.i] == '0' || s[lx.i] == '1'); ++lx.i)
            ++digits;
    } else {
        for (; lx.i < s.size() && std::isdigit((unsigned char)s[lx.i]); ++lx.i)
            ++digits; // 0123 is decimal 123: there is no octal
    }
    // `0x` with no digit, and a number glued to letters (`0xGG`, `12ab`,
    // `0b102`): the whole lexeme is wrong, not "a number and then a word".
    if (digits == 0 ||
        (lx.i < s.size() && (std::isalnum((unsigned char)s[lx.i]) || s[lx.i] == '_')))
        return fail(lx, "bad number");
    // The text as written; how big it is is the assembler's question.
    emit(lx, Token::Kind::Number, s.substr(start, lx.i - start));
    return true;
}

// Lab 8 Advanced: a "..." or '...' literal from the opening quote to the
// closing one, as written. A backslash takes the next character with it
// (\n \t \0 \\ \" \'); the line must not end before the quote does.
static bool quoted(Lexer& lx, char quote, Token::Kind kind) {
    const std::string& s = lx.src;
    const std::size_t start = lx.i++;
    std::size_t chars = 0;
    while (lx.i < s.size() && s[lx.i] != quote && s[lx.i] != '\n') {
        if (s[lx.i] == '\\') {
            const char e = lx.i + 1 < s.size() ? s[lx.i + 1] : '\n';
            if (e != 'n' && e != 't' && e != '0' && e != '\\' && e != '"' && e != '\'')
                return fail(lx, e == '\n' ? std::string("bad escape at the end of the line")
                                          : std::string("bad escape \\") + e);
            ++lx.i;
        }
        ++lx.i;
        ++chars;
    }
    if (lx.i >= s.size() || s[lx.i] != quote)
        return fail(lx, quote == '"' ? "unterminated string" : "unterminated character");
    ++lx.i;
    if (kind == Token::Kind::Char && chars != 1)
        return fail(lx,
                    "a character literal holds one character: " + s.substr(start, lx.i - start));
    emit(lx, kind, s.substr(start, lx.i - start));
    return true;
}

Token* lex(const std::string& src, std::string& error) {
    Lexer lx{src, 0, 1, nullptr, nullptr, ""};
    while (lx.i < src.size()) {
        char c = src[lx.i];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++lx.i; // пробіли не токени
        } else if (c == ';') {
            while (lx.i < src.size() && src[lx.i] != '\n')
                ++lx.i; // коментар
        } else if (c == '\n') {
            emit(lx, Token::Kind::Newline, "\\n");
            ++lx.line;
            ++lx.i;
        } else if (std::isalpha((unsigned char)c) || c == '_') {
            std::size_t start = lx.i;
            while (lx.i < src.size() &&
                   (std::isalnum((unsigned char)src[lx.i]) || src[lx.i] == '_')) {
                ++lx.i;
            }
            emit(lx, Token::Kind::Ident, src.substr(start, lx.i - start));
        } else if (std::isdigit((unsigned char)c)) {
            if (!number(lx))
                break;
        } else if (c == ',') {
            emit(lx, Token::Kind::Comma, ",");
            ++lx.i;
        } else if (c == ':') {
            emit(lx, Token::Kind::Colon, ":");
            ++lx.i;
        } else if (c == '[') {
            emit(lx, Token::Kind::LBracket, "[");
            ++lx.i;
        } else if (c == ']') {
            emit(lx, Token::Kind::RBracket, "]");
            ++lx.i;
        }
        // Lab 8 Advanced: a directive is a dot glued to a word (.org, .byte).
        else if (c == '.' && lx.i + 1 < src.size() && std::isalpha((unsigned char)src[lx.i + 1])) {
            std::size_t start = lx.i++;
            while (lx.i < src.size() &&
                   (std::isalnum((unsigned char)src[lx.i]) || src[lx.i] == '_')) {
                ++lx.i;
            }
            emit(lx, Token::Kind::Directive, src.substr(start, lx.i - start));
        }
        // Lab 8 Advanced: string and character literals.
        else if (c == '"') {
            if (!quoted(lx, '"', Token::Kind::String))
                break;
        } else if (c == '\'') {
            if (!quoted(lx, '\'', Token::Kind::Char))
                break;
        }
        // Lab 8 Advanced: the operators of expressions.
        else if (c == '+') {
            emit(lx, Token::Kind::Plus, "+");
            ++lx.i;
        } else if (c == '-') {
            emit(lx, Token::Kind::Minus, "-");
            ++lx.i;
        } else if (c == '*') {
            emit(lx, Token::Kind::Star, "*");
            ++lx.i;
        } else if (c == '(') {
            emit(lx, Token::Kind::LParen, "(");
            ++lx.i;
        } else if (c == ')') {
            emit(lx, Token::Kind::RParen, ")");
            ++lx.i;
        } else {
            fail(lx, std::string("unexpected character '") + c + "'");
            break;
        }
    }
    if (!lx.error.empty()) {
        destroy(lx.head);
        error = lx.error;
        return nullptr;
    }
    emit(lx, Token::Kind::Eof, "");
    return lx.head;
}
