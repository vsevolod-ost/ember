// sprite.cpp — moving and drawing the host's sprites. Drawing goes through
// the same clear/plot as CLS/PLOT, so mem_get/mem_set stay the only code
// that touches Memory::data.
#include "sprite.hpp"

#include "display.hpp"

// One axis: pos + v, mirrored at 0 and at `last` (63 for x, 31 for y).
// Counted in int: 63 + 63 does not fit in an int8_t, and must not wrap.
static void move_axis(std::uint8_t& pos, std::int8_t& v, int last) {
    int p = pos + v, nv = v;
    if (p < 0) { // past the left/top edge: as far back inside
        p = -p;
        nv = -nv;
    } else if (p > last) { // past the right/bottom edge
        p = 2 * last - p;
        nv = -nv;
    }
    // Exactly on an edge and still heading out: turn now (notes §5: on the
    // edge the speed flips), so the next tick moves inside.
    if ((p == 0 && nv < 0) || (p == last && nv > 0))
        nv = -nv;
    pos = static_cast<std::uint8_t>(p);
    v = static_cast<std::int8_t>(nv); // -63..63: fits, nothing is cut
}

void sprite_move(Sprite& s) {
    move_axis(s.x, s.vx, SCREEN_W - 1);
    move_axis(s.y, s.vy, SCREEN_H - 1);
}

void sprites_tick(Sprite sprites[]) {
    for (int i = 0; i < SPRITE_COUNT; ++i) {
        if (sprites[i].alive)
            sprite_move(sprites[i]);
    }
}

void sprites_draw(Memory& mem, const Sprite sprites[]) {
    clear(mem);
    for (int i = 0; i < SPRITE_COUNT; ++i) {
        if (sprites[i].alive)
            plot(mem, sprites[i].x, sprites[i].y);
    }
}

int sprites_alive(const Sprite sprites[]) {
    int n = 0;
    for (int i = 0; i < SPRITE_COUNT; ++i) {
        if (sprites[i].alive)
            ++n;
    }
    return n;
}
