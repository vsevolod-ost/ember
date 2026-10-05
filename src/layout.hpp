// layout.hpp — `layout`: where each field of a host struct lives (Lab 6).
#pragma once

#include <string>

// One table per struct: offset and size of every field from offsetof and
// sizeof, the padding between and after them, and the sum. `what` is cpu,
// flags, instruction, sprite or value; "" prints all five. False (nothing
// printed) for any other word.
bool print_layout(const std::string& what);
