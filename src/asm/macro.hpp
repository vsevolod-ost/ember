// macro.hpp — `.define NAME tokens...` (Lab 8 Advanced): pass 0 of the
// assembler, done on the token list before any address is counted.
//
// A macro is tokens, not a value: every later Ident(NAME) is replaced by a
// copy of the tokens after NAME, up to the end of the .define line. So
// `.define N 2+3` makes `N*2` the tokens 2+3*2, which is 8, not 10; write
// `.define N (2+3)` for 10. A body is expanded where it is defined (macros
// used in it must come before it), so a macro cannot call itself.
#pragma once

#include <string>

#include "token.hpp"

// Replace the uses, and cut each .define line out of the list (its Newline
// stays, so the line is still counted). False and `error` ("line N: ...")
// on a bad .define; the list is then still whole, for the caller to destroy.
bool expand_defines(Token*& head, std::string& error);
