// lexer.hpp — GIVEN (Lab 8, M1): text in, a list of tokens or an error out.
#pragma once

#include <string>

#include "token.hpp"

Token* lex(const std::string& src, std::string& error);
