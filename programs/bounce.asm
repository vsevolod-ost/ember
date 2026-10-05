; bounce.asm — піксель їде екраном і відбивається від стінок (Advanced)
.define FRAMES 5             ; скільки кадрів показати
.define WIDTH  64
.define HEIGHT 32
.define X  0                 ; поля м'ячика: [ball+X], [ball+Y], ...
.define Y  1
.define DX 2                 ; швидкість: +1 або -1 (0xff)
.define DY 3
        CLS
frame:  LOAD  A, [ball+X]
        LOAD  B, [ball+Y]
        PLOT
        SHOW
        UNPLOT               ; A і B ще x і y: стерти перед кроком

        LOAD  B, [ball+DX]   ; x += dx
        ADD   A, B
        STORE [ball+X], A
        LOADI B, 0x100-WIDTH ; біти, яких у 0..WIDTH-1 немає
        AND   A, B
        JZ    x_ok           ; ще на екрані
        LOADI A, 0           ; вилетів: dx = -dx,
        LOAD  B, [ball+DX]
        SUB   A, B
        STORE [ball+DX], A
        MOV   B, A
        ADD   A, B           ; x += 2 * dx: назад і крок від стінки
        LOAD  B, [ball+X]
        ADD   A, B
        STORE [ball+X], A
x_ok:   LOAD  A, [ball+Y]    ; y += dy, так само
        LOAD  B, [ball+DY]
        ADD   A, B
        STORE [ball+Y], A
        LOADI B, 0x100-HEIGHT
        AND   A, B
        JZ    y_ok
        LOADI A, 0
        LOAD  B, [ball+DY]
        SUB   A, B
        STORE [ball+DY], A
        MOV   B, A
        ADD   A, B
        LOAD  B, [ball+Y]
        ADD   A, B
        STORE [ball+Y], A
y_ok:   LOAD  A, [left]      ; ще кадр?
        DEC   A
        STORE [left], A
        JNZ   frame
        HALT

        .org 0x800
ball:   .byte 61, 1, 1, -1   ; x, y, dx, dy: біля правого верхнього кута
left:   .byte FRAMES
