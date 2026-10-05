; search.asm — лінійний пошук з Lab 04: де в [LO, LO + 8) лежить NEEDLE
.define LO     0x0800
.define NEEDLE 0x41
        LOADH H, LO          ; i = lo
        LOADI B, NEEDLE      ; B більше не змінюється
loop:   LOAD  A, [H]         ; A = mem[i]
        CMP   A, B
        JZ    found
        INCH                 ; ++i
        HLOW
        SHR   A
        SHR   A
        SHR   A              ; A = i / 8: 0, поки i < 8
        JZ    loop
        LOADI A, '-'         ; промах
        OUT
        HALT
found:  HLOW                 ; A = індекс
        OUTN
        HALT

        .org LO
table:  .byte 0x10, 0x41, 0x30, 0x41, 0, 0, 0, 0
