; вирази в операндах
        LOADI A, 2+3
        LOADI A, 2+3*4
        LOADI A, (2+3)*4
        LOADI A, -1
        LOAD  A, [data+1]
        HALT
        .org 0x800
data:   .byte 7, 42
