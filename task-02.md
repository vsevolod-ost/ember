Лаб 2. Біти не брешуть: системи числення, прапорці й ALU

Продовження ember у тому самому репозиторії: коробка на 4 КБ, dump / get / set з лаби 1 уже є. Цього тижня байтами оперуєте, і з’являється процесор, який уміє один step.
 Термін: 2 тижні
Що вивчаємо: двійкова й hex, доповняльний код, бітові оператори (& | ^ ~ << >>), маски й зсуви, прапорці Z/N/C, пріоритет операцій, розбір опкода.
Що робимо:
ALU як функції над uint8_t: ADD, SUB, AND, OR, XOR, NOT, SHL, SHR, INC, DEC — кожна повертає значення й виставляє прапорці. Wrap на 8 бітах. У README два розібрані приклади з перенесенням (обов’язково 200 + 100 → result=44 C=1 і ще один свій).
CPU: регістри A, B, PC, прапорці. Команда regs друкує PC A B Z N C. Через set кладете байти програми, step виконує одну інструкцію. HALT зупиняє подальші кроки.
Реалізуєте групи 0x0_ і 0x1_ з таблиці ISA — опкоди, розмір у байтах і вплив на прапорці берете з таблиці, не вигадуєте. decode() дістає групу як (op >> 4) & 0x0F, а не магічне число на кожну інструкцію.
Трейс програми з 3+ інструкцій у README: у кожному рядку PC, опкод, нове A і прапорці. Баг x & MASK == 0 без дужок — зустріли або показали навмисно.
Що здаємо: той самий публічний GitHub-репозиторій з тегом lab-02; README з прикладами ALU, трейсом і переліком рядків ISA, які вже живі. Захист — 5 хвилин: set → step → regs + 2–3 питання з розділу «На захисті».

Повний текст лаби: https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/lab-02-bits-dont-lie.md
Нотатки (теорія + досліди): https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/lab-02-bits-dont-lie.notes.md
Таблиця інструкцій (контракт на семестр): https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md

Прикріплене посилання: https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/lab-02-bits-dont-lie.md