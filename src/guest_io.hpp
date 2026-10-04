// guest_io.hpp — what the guest program prints (OUT, OUTN).
//
// Guest output and the host's own lines (trace, "halted after ...", errors)
// share one terminal. The guest does not end its lines, so before every host
// line call guest_end_line(): it starts a new line only if the guest left one
// open. That keeps `AB` and `halted after 7 steps` on separate lines.
#pragma once

#include "memory.hpp"

// OUT: the byte as it is, a raw character (even NUL or '\n').
void guest_putc(Byte b);

// OUTN: the byte as a decimal number, then a space.
void guest_putn(Byte b);

// Print '\n' if the last guest byte left the line open; otherwise nothing.
void guest_end_line();
