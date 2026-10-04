// guest_io.cpp — guest output, plus the one fact the host needs about it:
// is the guest's current line still open?
#include "guest_io.hpp"

#include <iostream>

// How many guest characters are on the current line. Only "zero or not"
// matters; it is reset by a guest '\n' and by guest_end_line().
static std::size_t column = 0;

void guest_putc(Byte b) {
    std::cout.put(static_cast<char>(b));
    column = (b == '\n') ? 0 : column + 1;
}

void guest_putn(Byte b) {
    std::cout << static_cast<int>(b) << ' ';
    column += 1;
}

void guest_end_line() {
    if (column != 0)
        std::cout << '\n';
    column = 0;
}
