; fib.asm — рекурсивний Фібоначчі на CALL/RET, конвенція ISA §6:
; аргумент в A, результат в A; B псується (його зберігає той, хто викликає).
.define N 6                  ; змініть тут: fib(N)
        LOADI A, N
        CALL  fib
        OUTN
        HALT

fib:    LOADI B, 0
        CMP   A, B           ; n == 0 ?
        JZ    ret_a          ; fib(0) = 0
        LOADI B, 1
        CMP   A, B           ; n == 1 ?
        JZ    ret_a          ; fib(1) = 1
        PUSH  A              ; зберегти n
        DEC   A
        CALL  fib            ; A = fib(n-1)
        POP   B              ; B = n
        PUSH  A              ; зберегти fib(n-1)
        MOV   A, B
        DEC   A
        DEC   A
        CALL  fib            ; A = fib(n-2)
        POP   B              ; B = fib(n-1)
        ADD   A, B
ret_a:  RET
