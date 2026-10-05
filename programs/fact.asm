LOADI A, 5        ; n = 5
CALL  fact
OUTN              ; надрукує 120
HALT

; fact(n): аргумент в A, результат в A. Псує B.
fact:  LOADI B, 0
       CMP   A, B       ; n == 0 ?
       JZ    base
       LOADI B, 1
       CMP   A, B       ; n == 1 ?
       JZ    base
       PUSH  A          ; зберегти n — виклик знищить A
       DEC   A
       CALL  fact       ; A = fact(n-1)
       POP   B          ; B = збережене n
       MUL   A, B       ; A = fact(n-1) * n
       RET
base:  LOADI A, 1
       RET
