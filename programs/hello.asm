; hello.asm — OUTS рядка, зібраного в регіон даних (0x800)
        OUTS msg
        HALT

        .org 0x800
msg:    .string "Hello, ember!\n"
