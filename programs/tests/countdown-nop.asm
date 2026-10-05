; той самий відлік, але першим рядком NOP
        NOP
start:  LOADI A, 3
loop:   OUTN
        DEC   A
        JNZ   loop
        HALT
