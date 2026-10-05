; зворотний відлік ISA §9 з мітками
start:  LOADI A, 3
loop:   OUTN
        DEC   A
        JNZ   loop
        HALT
