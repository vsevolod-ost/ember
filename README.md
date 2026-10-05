# `ember` — starter skeleton (Lab 1)

Copy this folder to a repository of your own, `git init`, and start from **M1** of
[Lab 01](../lab-01-a-box-of-bytes.md). It builds and runs as-is.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/ember
```

## Why a skeleton exists

Lab 1 is about **bytes and types**. It is not about `while` loops, splitting a
string into words, or `std::setw` — you meet those properly in Labs 4, 5 and 7.
So the parts that need them are given to you, fully written and commented. You
read those. You write the four small things that *are* Lab 1.

## Given — read it, don't rewrite it

| File | What it does |
|---|---|
| `CMakeLists.txt` | C++17, `-Wall -Wextra -Werror`, ASan + UBSan on Debug |
| `src/main.cpp` | the prompt: read a line, split it into words, call your functions |
| `src/memory.hpp` | `Byte`, `MEM_SIZE`, `struct Memory` — the box |
| `src/dump.hpp` | the two declarations |
| `src/dump.cpp` → `dump()` | the hex dump loop |

## Yours — four `TODO(lab-01)` markers

```bash
grep -rn "TODO(lab-01)" src/
```

| # | Where | The job |
|---|---|---|
| 1 | `memory.cpp` → `mem_get` | return the byte, or 0 if the address is outside the box |
| 2 | `memory.cpp` → `mem_set` | write the byte, or return `false` if the address is outside |
| 3 | `dump.cpp` → the ASCII gutter | print the character when the byte is printable |
| 4 | `dump.cpp` → `show_byte` | one byte, four views |

When all four are done:

```txt
ember> set 0 65
ember> set 1 66
ember> get 0
65  0x41  0b01000001  'A'
ember> dump
0000  41 42 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |AB..............|
```

That is M2 and M3 of Lab 1. M4 (the three deliberate breakages) is in the lab.

Until you implement `mem_set`, `set` accepts everything and stores nothing, and
`get` prints `show_byte: not implemented yet`. That is the starting state, not a
bug.

## Later labs

You keep this repository for all eight labs. Every lab adds one `else if` branch
to the dispatcher in `main.cpp` and one or two new files next to these.

---

## Українською

Скопіюйте цю теку у свій репозиторій — вона вже збирається й запускається.

Lab 1 — про **байти й типи**, а не про цикли, розбір рядка на слова чи
форматування виводу (це Labs 4, 5, 7). Тому все, що потребує ще не пройденого,
вам **дано** — з коментарями, щоб читати. Ви пишете чотири маленькі речі, які й
є Lab 1: `mem_get`, `mem_set`, ASCII-колонку в дампі та `show_byte`.

Знайти свою роботу: `grep -rn "TODO(lab-01)" src/`.

Якщо C++ бачите вперше — спочатку
[C++ за годину](../cpp-survival-kit.notes.md), потім
[інструменти й git](../setup.notes.md).

---

## M1 — вітання і prompt

```txt
ember 0.1 - 4096 bytes of memory you can see. Type `help`.
ember>
```

## M2 — порожня коробка

```txt
ember> dump
0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0020  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
```

## M3 — прочитати й записати байт

```txt
ember> set 0 65
ember> get 0
65  0x41  0b01000001  'A'
ember> set 5000 1
address 5000 is outside 0..4095
```

## M4 — навмисні поломки

### 1. `inc` і загортання: unsigned wrap vs signed UB

`inc <addr>` читає байт, додає одиницю й пише назад. `255 + 1` дало `0`, а не `256` — байт **загорнувся**, і для `std::uint8_t` це визначена поведінка:

```txt
ember> set 0 255
ember> get 0
255  0xff  0b11111111  '.'
ember> inc 0
ember> get 0
0  0x00  0b00000000  '.'
```

Той самий дослід із notes §2 для **знакового** `int`, зібраний з UBSan:

```cpp
#include <iostream>
#include <cstdint>
int main() {
    std::uint8_t u = 255;
    u = static_cast<std::uint8_t>(u + 1);
    std::cout << (int)u << '\n';          // 0

    int s = 2147483647;
    s = s + 1;                            // UBSan: signed overflow
    std::cout << s << '\n';
}
```

```txt
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined scratch.cpp -o scratch && ./scratch
0
scratch.cpp:9:7: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
-2147483648
```

Знаковий `int` переповнився: невизначена поведінка, і UBSan доповів про неї в момент виконання.

### 2. `0.1 + 0.2 == 0.3` → `false`

```cpp
#include <iostream>
int main() {
    std::cout << (0.1 + 0.2) << '\n';
    std::cout << std::boolalpha << (0.1 + 0.2 == 0.3) << '\n';
}
```

```txt
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined scratch.cpp -o scratch && ./scratch
0.3
false
```

`0.1`, `0.2` і `0.3` не мають точного двійкового запису — кожен округлюється до найближчого `double`. Сума двох округлених чисел не зобов'язана збігтися біт-у-біт з окремо округленим `0.3`; на екрані `0.3` — це вже округлення при друці.

### 3. `"AB\0"` у дампі

```txt
ember> set 0 65
ember> set 1 66
ember> set 2 0
ember> dump
0000  41 42 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |AB..............|
```

У пам'яті лежить C-рядок `"AB"`: два символи й нуль-термінатор.

### 4. Прибрані `{}` — неініціалізована пам'ять

Прибрав `{}` та отримав ті самі нулі: AddressSanitizer у Debug-збірці переносить великі локальні масиви у fake stack.
Запуск без fake stack, щоб зловити сміття:
```txt
ASAN_OPTIONS=detect_stack_use_after_return=0 ./build/ember
ember> dump
0160  01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0170  88 f0 0e b8 fd 7f 00 00 e0 8c 84 e2 c9 7f 00 00  |................|
0180  60 f0 0e b8 fd 7f 00 00 41 2c 83 e2 c9 7f 00 00  |`.......A,......|
0190  00 00 00 00 00 00 00 00 00 50 7e e2 c9 7f 00 00  |.........P~.....|
01a0  26 00 00 00 00 00 00 00 01 ad 0c 01 00 00 00 00  |&...............|
```

## `sizeof` на моїй машині (notes §1)

```txt
1 4 4 8 8
1 2
```

| Тип | `sizeof`, байт |
|---|---|
| `char` | 1 |
| `int` | 4 |
| `float` | 4 |
| `double` | 8 |
| `void*` | 8 |
| `std::uint8_t` | 1 |
| `std::uint16_t` | 2 |

## Чому клітинка `ember` — `std::uint8_t`, а не `int`

1. **Стала ширина.** `sizeof(std::uint8_t) == 1` гарантовано на кожній платформі, а `sizeof(int)` — ні.
2. **Визначене переповнення.** Беззнакове загортання (`255 + 1 == 0`) — визначена арифметика за модулем 256; переповнення знакового `int` — UB (дослід M4 №1).

## Advanced

### `set16` і порядок байтів

`set16 <addr> <value>` кладе 16-бітне значення (`0..65535`) у дві сусідні клітинки. Межа перевіряється до запису: якщо друга клітинка виходить за `4095`, не пишеться жодна.

```txt
ember> set16 0 0x1234
ember> dump
0000  34 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |4...............|
ember> set16 4095 1
set16 needs two bytes at 4095 and 4096; the box is 0..4095
```

Молодший байт `0x34` ліг першим, за нижчою адресою `0`, старший `0x12` — за адресою `1`: ця машина (x86-64) little-endian. Big-endian машина поклала б `12 34`.

### `.clang-format`

Файл зі скелета застосовано до всього `src/`: `clang-format -i src/*.cpp src/*.hpp`.

### За бажанням

```cpp
int main() {
    int x = 67;
    return x;
}
```

x86-64 gcc, `-O0`, [Compiler Explorer](https://godbolt.org/):

```asm
main:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-4], 67
        mov     eax, DWORD PTR [rbp-4]
        pop     rbp
        ret
```

- `mov DWORD PTR [rbp-4], 67` кладе літерал `67` у `x` на стеку;
- `mov eax, DWORD PTR [rbp-4]` читає ці самі чотири байти назад у регістр `eax` для `return x`.

---

# Lab 02 — Біти не брешуть

## M1 — ALU: два розібрані приклади

```txt
ember> alu add 200 100
result=44  Z=0 N=0 C=1
```

```txt
    11001000   200
  + 01100100   100
  ----------
  1 00101100   300 = 256 + 44  -> result = 44, дев'ятий біт іде в C
```

```txt
ember> alu shl 200
result=144  Z=0 N=1 C=1
```

```txt
    11001000   200
 << 1
  ----------
  1 10010000   400 = 256 + 144  -> result = 144; старший біт виїхав у C = 1, новий біт 7 = 1 -> N = 1
```

Те саме для `AND` (перенесення немає, `C` скидається):

```txt
ember> alu and 200 100
result=64  Z=0 N=0 C=0
```

```txt
    11001000   200
  & 01100100   100
  ----------
    01000000    64
```

`ADD` зібраний із бітів (`src/alu.cpp`, цикл із notes §5); у Debug-збірці `assert` звіряє результат і перенесення з `uint8_t(a + b)`.

## M2 — `reg` → `set` → `step` → `regs`

`A=200`, `B=100`, у пам'яті `ADD A, B` і `HALT`:

```txt
ember> reg a 200
ember> reg b 100
ember> set 0 0x10
ember> set 1 0x00
ember> regs
PC=0x0000  A=200  B=100  Z=0 N=0 C=0
ember> step
0000  10     ADD A, B -> A= 44  Z=0 N=0 C=1
ember> regs
PC=0x0001  A=44  B=100  Z=0 N=0 C=1
ember> step
0001  00     HALT     -> A= 44  Z=0 N=0 C=1
ember> step
halted: step does nothing after HALT
ember> regs
PC=0x0002  A=44  B=100  Z=0 N=0 C=1
```

## M3 — трасування

Рядок трасування: `PC`, байт опкода, мнемоніка, нове `A`, прапорці.

`A=7`, `B=1`, байти `[AND, HALT]`:

```txt
ember> reg a 7
ember> reg b 1
ember> set 0 0x12
ember> set 1 0x00
ember> step
0000  12     AND A, B -> A=  1  Z=0 N=0 C=0
ember> step
0001  00     HALT     -> A=  1  Z=0 N=0 C=0
ember> regs
PC=0x0002  A=1  B=1  Z=0 N=0 C=0
```

`SHL`, поки не з'явиться перенесення, потім `INC` — він `C` не чіпає:

```txt
ember> reg a 0x41
ember> set 0 0x16
ember> set 1 0x16
ember> set 2 0x18
ember> set 3 0x00
ember> run
0000  16     SHL A    -> A=130  Z=0 N=1 C=0
0001  16     SHL A    -> A=  4  Z=0 N=0 C=1
0002  18     INC A    -> A=  5  Z=0 N=0 C=1
0003  00     HALT     -> A=  5  Z=0 N=0 C=1
ember> regs
PC=0x0004  A=5  B=0  Z=0 N=0 C=1
ember> get 0xB00
1  0x01  0b00000001  '.'
```

## M4 — розбір опкода маскою і зсувом

`src/cpu.cpp`:

```cpp
Decoded decode(Byte op) {
    return {static_cast<Byte>((op >> 4) & 0x0F), static_cast<Byte>(op & 0x0F)};
}
```

`step()` спершу розводить `switch` за групою (`0x0_`, `0x1_`, `0x2_`), а вже всередині групи — за `Op` з `src/opcodes.hpp`, значення якого взяті з ISA.

## Реалізовані рядки [ISA.uk.md](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md)

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x00` | `HALT` | 1 | — | зупинитись; наступні `step` відмовляють | 2 |
| `0x01` | `NOP` | 1 | — | нічого | 2 |
| `0x10` | `ADD A, B` | 1 | Z N C | `A = A + B` | 2 |
| `0x11` | `SUB A, B` | 1 | Z N C | `A = A - B` | 2 |
| `0x12` | `AND A, B` | 1 | Z N, C=0 | `A = A & B` | 2 |
| `0x13` | `OR A, B` | 1 | Z N, C=0 | `A = A \| B` | 2 |
| `0x14` | `XOR A, B` | 1 | Z N, C=0 | `A = A ^ B` | 2 |
| `0x15` | `NOT A` | 1 | Z N, C=0 | `A = ~A` | 2 |
| `0x16` | `SHL A` | 1 | Z N C | `A = A << 1`; у `C` потрапляє біт, що виїхав із сьомої позиції | 2 |
| `0x17` | `SHR A` | 1 | Z N C | `A = A >> 1`; у `C` потрапляє біт, що виїхав із нульової | 2 |
| `0x18` | `INC A` | 1 | Z N | `A = A + 1` | 2 |
| `0x19` | `DEC A` | 1 | Z N | `A = A - 1` | 2 |
| `0x20` | `LOADI A, imm8` | 2 | — | `A = imm8` | 3 |
| `0x21` | `LOADI B, imm8` | 2 | — | `B = imm8` | 3 |

`LOADI A/B` — рядки Лаби 3, реалізовані заздалегідь разом із командою `run`. Значення ззовні в регістри кладе дана команда `reg`.

## Власні розширення

| Регіон | Діапазон | Константа | З'являється в |
|---|---|---|---|
| Байт стану | `0xB00` | `STATUS_ADDR` | Лаба 2 |

Байт стану після кожного успішного `step` дзеркалить прапорці: `0b00000ZNC` (біт 2 — `Z`, біт 1 — `N`, біт 0 — `C`). Дзеркало одностороннє: запис у `0xB00` прапорці не змінює.

---

# Lab 03 — Адреси, а не імена

Нові й змінені команди REPL (щоб транскрипти нижче читались):

| Команда | Що робить |
|---|---|
| `get16 <addr>` | прочитати 16-бітне значення, молодший байт перший: `4660  0x1234` |
| `dump <addr> [n]` | рядки дампа, що містять `n` байтів від `addr` (типово 16); рядки вирівняні по 16. `dump` без аргументів — як раніше, усі 4096 байтів |
| `reg pc <v>`, `reg h <v>` | покласти 0..65535 у `PC` або `H` (дана `reg a\|b` не змінена) |
| `run` | виконує до `HALT` або до помилки; друкує тільки вивід гостя, потім `halted after N steps` (`N` — разом із `HALT`) або повідомлення про помилку і `stopped after N steps` |
| `trace on` / `trace off` | `run` друкує ще й рядок трасування на кожен крок (типово вимкнено); `step` друкує його завжди |
| `hostdump cpu` | структура `CPU` хоста байтами: `dump(&cpu, sizeof(cpu))` |
| `get <addr>` | для адреси `>= 4096` тепер повідомлення, а не `0` |

`regs` друкує `PC A B H Z N C`. Рядок трасування: адреса інструкції, її байти (1–3), мнемоніка з підставленими операндами, потім `A`, `B`, `H` і прапорці після виконання. Вивід гостя (`OUT`, `OUTN`) іде до рядка трасування, на окремому рядку.

## M1 — `get16` / `set16`, молодший байт уперед

`src/memory.cpp`:

```cpp
// ДАНО. Прочитати два байти як одне 16-бітне значення, молодший перший.
std::uint16_t get16(const Memory& mem, std::size_t addr) {
    Byte lo = mem_get(mem, addr);
    Byte hi = mem_get(mem, addr + 1);
    return (std::uint16_t)(lo | (hi << 8));
}

bool set16(Memory& mem, std::size_t addr, std::uint16_t value) {
    // Both cells must fit. `addr >= MEM_SIZE - 1` is `addr + 1 >= MEM_SIZE`
    // without the + 1, so a huge addr cannot wrap around. Checked before any
    // write: a refused set16 changes nothing.
    if (addr >= MEM_SIZE - 1)
        return false;
    // The mirror of get16: low byte first, at the lower address.
    mem_set(mem, addr, static_cast<Byte>(value & 0xFF));
    mem_set(mem, addr + 1, static_cast<Byte>(value >> 8));
    return true;
}
```

```txt
ember> set16 0 0x1234
ember> dump 0 16
0000  34 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |4...............|
ember> get16 0
4660  0x1234
ember> set16 4095 1
set16 needs two bytes at 4095 and 4096; the box is 0..4095
ember> get 4095
0  0x00  0b00000000  '.'
ember> quit
```

За [ISA.uk.md §4](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md#4-кодування) кожен 16-бітний операнд у потоці інструкцій лежить молодшим байтом уперед, тому `LOADH H, 0x0A00` — це байти `28 00 0a`; `step` читає операнд тим самим `get16`:

```txt
ember> set 0 0x28
ember> set16 1 0x0A00
ember> dump 0 16
0000  28 00 0a 00 00 00 00 00 00 00 00 00 00 00 00 00  |(...............|
ember> step
0000  28 00 0a  LOADH H, 0x0a00   -> A=  0  B=  0  H=0x0a00  Z=0 N=0 C=0
ember> regs
PC=0x0003  A=0  B=0  H=0x0a00  Z=0 N=0 C=0
ember> quit
```

## M2 — група `0x2_`, `OUT`, `OUTN`

Розмір кожної інструкції — з таблиці ISA, в одному місці (`instr_size()` у `src/cpu.cpp`). Програма з усіма абсолютними пересиланнями:

```txt
адр.  байти     текст
0000  20 2A     LOADI A, 42
0002  24 01 08  STORE [0x0801], A
0005  21 07     LOADI B, 7
0007  23 01 08  LOAD B, [0x0801]     ; B = 42
000A  20 00     LOADI A, 0
000C  26        MOV A, B             ; A = B = 42
000D  03        OUTN                 ; "42 "
000E  25 02 08  STORE [0x0802], B
0011  00        HALT
```

```txt
ember> set 0 0x20
ember> set 1 42
ember> set 2 0x24
ember> set16 3 0x0801
ember> set 5 0x21
ember> set 6 7
ember> set 7 0x23
ember> set16 8 0x0801
ember> set 10 0x20
ember> set 11 0
ember> set 12 0x26
ember> set 13 0x03
ember> set 14 0x25
ember> set16 15 0x0802
ember> set 17 0x00
ember> trace on
ember> run
0000  20 2a     LOADI A, 0x2a     -> A= 42  B=  0  H=0x0000  Z=0 N=0 C=0
0002  24 01 08  STORE [0x0801], A -> A= 42  B=  0  H=0x0000  Z=0 N=0 C=0
0005  21 07     LOADI B, 0x07     -> A= 42  B=  7  H=0x0000  Z=0 N=0 C=0
0007  23 01 08  LOAD B, [0x0801]  -> A= 42  B= 42  H=0x0000  Z=0 N=0 C=0
000a  20 00     LOADI A, 0x00     -> A=  0  B= 42  H=0x0000  Z=0 N=0 C=0
000c  26        MOV A, B          -> A= 42  B= 42  H=0x0000  Z=0 N=0 C=0
42 
000d  03        OUTN              -> A= 42  B= 42  H=0x0000  Z=0 N=0 C=0
000e  25 02 08  STORE [0x0802], B -> A= 42  B= 42  H=0x0000  Z=0 N=0 C=0
0011  00        HALT              -> A= 42  B= 42  H=0x0000  Z=0 N=0 C=0
halted after 9 steps
ember> regs
PC=0x0012  A=42  B=42  H=0x0000  Z=0 N=0 C=0
ember> dump 0x800 16
0800  00 2a 2a 00 00 00 00 00 00 00 00 00 00 00 00 00  |.**.............|
ember> quit
```

Прапорці не змінюються в жодному рядку (ISA §3): навіть `LOADI A, 0x00` не виставляє `Z`.

### Межі

`step` не читає інструкцію за кінцем пам'яті — одна перевірка до будь-яких змін (`src/cpu.cpp`):

```cpp
    if (at + size > MEM_SIZE)
        return StepResult::out_of_memory;
```

Адресу гостя `>= 4096` у `LOAD`/`STORE` (і за `addr16`, і за `H`) машина відхиляє, а не загортає: повідомлення з адресою і `PC`, нічого не змінено, `run` зупиняється. `INCH`/`DECH` загортають `H` за модулем 65536 (`H` — `uint16_t`) без помилки: тримати погану адресу можна, помилка — лише коли нею користуються.

```txt
ember> reg pc 4094
ember> set 4094 0x22
ember> step
instruction at 0x0ffe does not fit in 0..4095; nothing done
ember> regs
PC=0x0ffe  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> reg pc 0
ember> set 0 0x20
ember> set 1 1
ember> set 2 0x21
ember> set 3 2
ember> set 4 0x22
ember> set16 5 0x1000
ember> run
address 0x1000 is outside 0..4095 (LOAD A, [0x1000] at 0x0004); nothing done
stopped after 2 steps
ember> regs
PC=0x0004  A=1  B=2  H=0x0000  Z=0 N=0 C=0
ember> set 4 0x2C
ember> step
0004  2c        DECH              -> A=  1  B=  2  H=0xffff  Z=0 N=0 C=0
ember> set 5 0x2A
ember> step
address 0xffff is outside 0..4095 (STORE [H], A at 0x0005); nothing done
ember> regs
PC=0x0005  A=1  B=2  H=0xffff  Z=0 N=0 C=0
ember> quit
```

## M3 — програма, яка користується даними

Код на `0x000`, дані `65 66 67 0` («ABC») на `0x800`.

### За абсолютною адресою

```txt
адр.  байти     текст
0000  22 00 08  LOAD A, [0x0800]
0003  02        OUT
0004  00        HALT
```

```txt
ember> set 0x800 65
ember> set 0x801 66
ember> set 0x802 67
ember> set 0x803 0
ember> set 0 0x22
ember> set16 1 0x0800
ember> set 3 0x02
ember> set 4 0x00
ember> run
A
halted after 3 steps
ember> dump 0 16
0000  22 00 08 02 00 00 00 00 00 00 00 00 00 00 00 00  |"...............|
ember> dump 0x800 16
0800  41 42 43 00 00 00 00 00 00 00 00 00 00 00 00 00  |ABC.............|
ember> quit
```

### Через `H`

```txt
адр.  байти     текст
0000  28 00 08  LOADH H, 0x0800
0003  29        LOAD A, [H]
0004  02        OUT
0005  2B        INCH
0006  29        LOAD A, [H]
0007  02        OUT
0008  00        HALT
```

```txt
ember> set 0x800 65
ember> set 0x801 66
ember> set 0x802 67
ember> set 0x803 0
ember> set 0 0x28
ember> set16 1 0x0800
ember> set 3 0x29
ember> set 4 0x02
ember> set 5 0x2B
ember> set 6 0x29
ember> set 7 0x02
ember> set 8 0x00
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> step
0000  28 00 08  LOADH H, 0x0800   -> A=  0  B=  0  H=0x0800  Z=0 N=0 C=0
ember> regs
PC=0x0003  A=0  B=0  H=0x0800  Z=0 N=0 C=0
ember> step
0003  29        LOAD A, [H]       -> A= 65  B=  0  H=0x0800  Z=0 N=0 C=0
ember> regs
PC=0x0004  A=65  B=0  H=0x0800  Z=0 N=0 C=0
ember> step
A
0004  02        OUT               -> A= 65  B=  0  H=0x0800  Z=0 N=0 C=0
ember> regs
PC=0x0005  A=65  B=0  H=0x0800  Z=0 N=0 C=0
ember> step
0005  2b        INCH              -> A= 65  B=  0  H=0x0801  Z=0 N=0 C=0
ember> regs
PC=0x0006  A=65  B=0  H=0x0801  Z=0 N=0 C=0
ember> step
0006  29        LOAD A, [H]       -> A= 66  B=  0  H=0x0801  Z=0 N=0 C=0
ember> regs
PC=0x0007  A=66  B=0  H=0x0801  Z=0 N=0 C=0
ember> step
B
0007  02        OUT               -> A= 66  B=  0  H=0x0801  Z=0 N=0 C=0
ember> regs
PC=0x0008  A=66  B=0  H=0x0801  Z=0 N=0 C=0
ember> step
0008  00        HALT              -> A= 66  B=  0  H=0x0801  Z=0 N=0 C=0
ember> regs
PC=0x0009  A=66  B=0  H=0x0801  Z=0 N=0 C=0
ember> quit
```

`H` змінюється рівно один раз — на `INCH`, з `0x0800` на `0x0801`; вивід `A`, потім `B`.

## M4 — вказівник хоста проти адреси гостя

```cpp
struct CPU {
    Memory* mem;
    std::uint16_t pc = 0; // address of the NEXT instruction
    std::uint16_t h = 0;  // address register: where LOAD A, [H] reads
    Byte a = 0, b = 0;
    Flags f{};
    bool halted = false; // set by HALT; every later step refuses
};
```

`CPU` тримає один вказівник хоста — `Memory* mem`, «де в моєму процесі лежить коробка», — а далі тільки адреси гостя: `pc` і `h` — числа `0x000`–`0xFFF`, індекси в `mem->data`. Таке число — частина машини: `H` видно в `regs`, його можна записати в пам'ять і прочитати назад, програма з ним рахує (`INCH`, `DECH`, `HLOW`), і та сама програма отримує ті самі адреси в кожному запуску й на будь-якому комп'ютері. `Byte*` на `mem->data[0x800]` — це адреса в просторі мого процесу: 8 байтів замість 2, у кожному запуску інша (два запуски `hostdump cpu` у розділі Advanced), і гостю вона нічого не каже. До того ж через індекс кожен доступ гостя проходить одну перевірку `addr < MEM_SIZE`, а `++p` на `Byte*` іде за кінець `data` без жодної перевірки.

Баг від змішування: зробити `PC` типу `Byte*` або покласти вказівник хоста в пам'ять гостя (`STORE` адреси буфера як числа). Вісім байтів не влазять у 16-бітний операнд і обрізаються до якоїсь іншої адреси; збережене значення неправдиве вже в наступному запуску (ASLR) і після перевиділення пам'яті; а `p + n` за межами `data` мовчки читає й пише чужу пам'ять хоста. Навпаки теж погано: адреса гостя `0x800`, прочитана як `Byte*` хоста, вказує невідомо куди в моєму процесі — розіменувати її означає UB.

### Навмисний вихід за межу

Тимчасовий `scratch.cpp` у корені проєкту (він у `.gitignore`):

```cpp
#include "memory.hpp"
int main() {
    Memory mem;
    std::size_t addr = MEM_SIZE;
    mem.data[addr] = 1;
}
```

```bash
clang++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I src scratch.cpp -o scratch && ./scratch
```

Індекс лежить у змінній, бо зі сталим індексом (`mem.data[MEM_SIZE] = 1;`) clang не збирає програму взагалі: `scratch.cpp:4:5: error: array index 4'096 is past the end of the array (that has type 'Byte[4096]' (aka 'unsigned char[4096]')) [-Werror,-Warray-bounds]`. Звіт ASan, як є:

```txt
=================================================================
==769292==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7bb5c9bf1020 at pc 0x56505dfa19f1 bp 0x7ffc2ae461b0 sp 0x7ffc2ae461a8
WRITE of size 1 at 0x7bb5c9bf1020 thread T0
    #0 0x56505dfa19f0 in main /home/user/Projects/ember/scratch.cpp:5:20
    #1 0x7fb5cb627780 in __libc_start_call_main /usr/src/debug/glibc/glibc/csu/../sysdeps/nptl/libc_start_call_main.h:59:16
    #2 0x7fb5cb6278b8 in __libc_start_main /usr/src/debug/glibc/glibc/csu/../csu/libc-start.c:372:3
    #3 0x56505de3b114 in _start (/home/user/Projects/ember/scratch+0x2d114) (BuildId: 9974e284e09593edb13318160b182a25fae35e06)

Address 0x7bb5c9bf1020 is located in stack of thread T0 at offset 4128 in frame
    #0 0x56505dfa1857 in main /home/user/Projects/ember/scratch.cpp:2

  This frame has 1 object(s):
    [32, 4128) 'mem' (line 3) <== Memory access at offset 4128 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow /home/user/Projects/ember/scratch.cpp:5:20 in main
Shadow bytes around the buggy address:
  0x7bb5c9bf0d80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7bb5c9bf0e00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7bb5c9bf0e80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7bb5c9bf0f00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7bb5c9bf0f80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x7bb5c9bf1000: 00 00 00 00[f3]f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
  0x7bb5c9bf1080: f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
  0x7bb5c9bf1100: f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
  0x7bb5c9bf1180: f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
  0x7bb5c9bf1200: f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
  0x7bb5c9bf1280: f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
==769292==ABORTING
```

1. `ERROR: AddressSanitizer: stack-buffer-overflow` — що сталось: вихід за межу об'єкта на стеку (`Memory mem` — локальна змінна `main`).
2. `WRITE of size 1` — писали, один байт (`Byte`).
3. `#0 … in main /home/user/Projects/ember/scratch.cpp:5:20` — мій рядок: `mem.data[addr] = 1;`.

Що зрозумів: `[32, 4128) 'mem'` — `mem` займає байти кадру 32..4127, а запис іде в 4128 = 32 + 4096, тобто в перший байт **після** `data[4095]`: останній дійсний індекс — `MEM_SIZE - 1`.

### Після: перевірка меж на місці

`scratch.cpp` і `scratch` видалені; у `src/` усі записи в пам'ять ідуть через `mem_set` (`src/memory.cpp`):

```cpp
bool mem_set(Memory& mem, std::size_t addr, Byte value) {
    // Out of range: change nothing and tell the caller via `false`.
    if (addr >= MEM_SIZE)
        return false;
    mem.data[addr] = value;
    return true;
}
```

```txt
ember> set 4096 1
address 4096 is outside 0..4095
ember> get 4096
address 4096 is outside 0..4095
ember> set 4095 1
ember> get 4095
1  0x01  0b00000001  '.'
ember> quit
```

## Advanced

### `dump(const void* ptr, std::size_t n)`

`src/hexdump.cpp` (`print_row` друкує один рядок у форматі даного `dump`: зміщення, 16 байтів, ASCII):

```cpp
void dump(const void* ptr, std::size_t n) {
    // A void* cannot be dereferenced or stepped: it has no element size.
    // Looking at it as bytes gives it one — and reading any object as
    // unsigned bytes is allowed by C++.
    const Byte* p = static_cast<const Byte*>(ptr);
    for (std::size_t off = 0; off < n; off += ROW)
        print_row(off, p + off, n - off < ROW ? n - off : ROW);
}
```

`void*` тут доречний: функції байдуже, *що* лежить за адресою, їй потрібно лише «скільки байтів». Два окремі запуски, та сама програма:

```txt
ember> set 0 0x28
ember> set16 1 0x0801
ember> step
0000  28 01 08  LOADH H, 0x0801   -> A=  0  B=  0  H=0x0801  Z=0 N=0 C=0
ember> regs
PC=0x0003  A=0  B=0  H=0x0801  Z=0 N=0 C=0
ember> hostdump cpu
&cpu=0x7bd6c1df10a0  sizeof(cpu)=24  sizeof(cpu.mem)=8  sizeof(*cpu.mem)=4096
0000  20 00 df c1 d6 7b 00 00 03 00 01 08 00 00 00 00  | ....{..........|
0010  00 00 00 00 00 00 00 00                          |........|
ember> quit
```

```txt
ember> set 0 0x28
ember> set16 1 0x0801
ember> step
0000  28 01 08  LOADH H, 0x0801   -> A=  0  B=  0  H=0x0801  Z=0 N=0 C=0
ember> regs
PC=0x0003  A=0  B=0  H=0x0801  Z=0 N=0 C=0
ember> hostdump cpu
&cpu=0x7b845adf10a0  sizeof(cpu)=24  sizeof(cpu.mem)=8  sizeof(*cpu.mem)=4096
0000  20 00 df 5a 84 7b 00 00 03 00 01 08 00 00 00 00  | ..Z.{..........|
0010  00 00 00 00 00 00 00 00                          |........|
ember> quit
```

| Зміщення | Поле | У дампі |
|---|---|---|
| 0–7 | `Memory* mem` | адреса хоста; у двох запусках різна |
| 8–9 | `std::uint16_t pc` | `03 00` = `0x0003` в обох |
| 10–11 | `std::uint16_t h` | `01 08` = `0x0801` в обох |
| 12 | `Byte a` | `00` |
| 13 | `Byte b` | `00` |
| 14–16 | `Flags f` (`bool z, n, c`) | `00 00 00` |
| 17 | `bool halted` | `00` |
| 18–23 | вирівнювання | — |

Полів 18 байтів, а `sizeof(cpu)` = 24: вказівник вимагає вирівнювання по 8, тож структура доповнена до кратного 8 (розкладка структур — Lab 6). `sizeof(cpu.mem)` = 8 — розмір адреси, `sizeof(*cpu.mem)` = 4096 — розмір того, на що вона вказує. `pc` і `h` лежать у хості так само, як `get16` читає їх у `ember`: молодший байт уперед.

### Копіювання чотирьох байтів через `H` і `MOV`

`0x800..0x803` → `0x810..0x813`, без жодної абсолютної адреси в `LOAD`/`STORE`. Байти йдуть парами: `MOV B, A` тримає перший, поки `H` іде за другим, `MOV A, B` повертає його для запису.

```txt
адр.  байти     текст
0000  28 00 08  LOADH H, 0x0800
0003  29        LOAD A, [H]       ; A = src[0]
0004  27        MOV B, A          ; B = src[0]
0005  2B        INCH              ; H = 0x0801
0006  29        LOAD A, [H]       ; A = src[1]
0007  28 11 08  LOADH H, 0x0811
000A  2A        STORE [H], A      ; dst[1] = src[1]
000B  2C        DECH              ; H = 0x0810
000C  26        MOV A, B          ; A = src[0]
000D  2A        STORE [H], A      ; dst[0] = src[0]
000E  28 02 08  LOADH H, 0x0802
0011  29        LOAD A, [H]       ; A = src[2]
0012  27        MOV B, A
0013  2B        INCH              ; H = 0x0803
0014  29        LOAD A, [H]       ; A = src[3]
0015  28 13 08  LOADH H, 0x0813
0018  2A        STORE [H], A      ; dst[3] = src[3]
0019  2C        DECH              ; H = 0x0812
001A  26        MOV A, B          ; A = src[2]
001B  2A        STORE [H], A      ; dst[2] = src[2]
001C  00        HALT
```

```txt
ember> set16 0x800 0x4D45
ember> set16 0x802 0x5242
ember> set 0x00 0x28
ember> set16 0x01 0x0800
ember> set 0x03 0x29
ember> set 0x04 0x27
ember> set 0x05 0x2B
ember> set 0x06 0x29
ember> set 0x07 0x28
ember> set16 0x08 0x0811
ember> set 0x0A 0x2A
ember> set 0x0B 0x2C
ember> set 0x0C 0x26
ember> set 0x0D 0x2A
ember> set 0x0E 0x28
ember> set16 0x0F 0x0802
ember> set 0x11 0x29
ember> set 0x12 0x27
ember> set 0x13 0x2B
ember> set 0x14 0x29
ember> set 0x15 0x28
ember> set16 0x16 0x0813
ember> set 0x18 0x2A
ember> set 0x19 0x2C
ember> set 0x1A 0x26
ember> set 0x1B 0x2A
ember> set 0x1C 0x00
ember> dump 0 32
0000  28 00 08 29 27 2b 29 28 11 08 2a 2c 26 2a 28 02  |(..)'+)(..*,&*(.|
0010  08 29 27 2b 29 28 13 08 2a 2c 26 2a 00 00 00 00  |.)'+)(..*,&*....|
ember> dump 0x800 32
0800  45 4d 42 52 00 00 00 00 00 00 00 00 00 00 00 00  |EMBR............|
0810  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> step
0000  28 00 08  LOADH H, 0x0800   -> A=  0  B=  0  H=0x0800  Z=0 N=0 C=0
ember> step
0003  29        LOAD A, [H]       -> A= 69  B=  0  H=0x0800  Z=0 N=0 C=0
ember> step
0004  27        MOV B, A          -> A= 69  B= 69  H=0x0800  Z=0 N=0 C=0
ember> step
0005  2b        INCH              -> A= 69  B= 69  H=0x0801  Z=0 N=0 C=0
ember> step
0006  29        LOAD A, [H]       -> A= 77  B= 69  H=0x0801  Z=0 N=0 C=0
ember> step
0007  28 11 08  LOADH H, 0x0811   -> A= 77  B= 69  H=0x0811  Z=0 N=0 C=0
ember> step
000a  2a        STORE [H], A      -> A= 77  B= 69  H=0x0811  Z=0 N=0 C=0
ember> step
000b  2c        DECH              -> A= 77  B= 69  H=0x0810  Z=0 N=0 C=0
ember> step
000c  26        MOV A, B          -> A= 69  B= 69  H=0x0810  Z=0 N=0 C=0
ember> step
000d  2a        STORE [H], A      -> A= 69  B= 69  H=0x0810  Z=0 N=0 C=0
ember> step
000e  28 02 08  LOADH H, 0x0802   -> A= 69  B= 69  H=0x0802  Z=0 N=0 C=0
ember> step
0011  29        LOAD A, [H]       -> A= 66  B= 69  H=0x0802  Z=0 N=0 C=0
ember> step
0012  27        MOV B, A          -> A= 66  B= 66  H=0x0802  Z=0 N=0 C=0
ember> step
0013  2b        INCH              -> A= 66  B= 66  H=0x0803  Z=0 N=0 C=0
ember> step
0014  29        LOAD A, [H]       -> A= 82  B= 66  H=0x0803  Z=0 N=0 C=0
ember> step
0015  28 13 08  LOADH H, 0x0813   -> A= 82  B= 66  H=0x0813  Z=0 N=0 C=0
ember> step
0018  2a        STORE [H], A      -> A= 82  B= 66  H=0x0813  Z=0 N=0 C=0
ember> step
0019  2c        DECH              -> A= 82  B= 66  H=0x0812  Z=0 N=0 C=0
ember> step
001a  26        MOV A, B          -> A= 66  B= 66  H=0x0812  Z=0 N=0 C=0
ember> step
001b  2a        STORE [H], A      -> A= 66  B= 66  H=0x0812  Z=0 N=0 C=0
ember> step
001c  00        HALT              -> A= 66  B= 66  H=0x0812  Z=0 N=0 C=0
ember> dump 0x800 32
0800  45 4d 42 52 00 00 00 00 00 00 00 00 00 00 00 00  |EMBR............|
0810  45 4d 42 52 00 00 00 00 00 00 00 00 00 00 00 00  |EMBR............|
ember> regs
PC=0x001d  A=66  B=66  H=0x0812  Z=0 N=0 C=0
ember> quit
```

## Якщо встигаєте

`dump(&cpu, sizeof(cpu))` і копіювання чотирьох байтів тільки через `H` і `MOV` — у розділі Advanced вище.

### `HLOW` — наскільки далеко зайшов `H`

```txt
адр.  байти     текст
0000  28 00 08  LOADH H, 0x0800
0003  29        LOAD A, [H]
0004  02        OUT
0005  2B        INCH
0006  29        LOAD A, [H]
0007  02        OUT
0008  2B        INCH
0009  2D        HLOW              ; A = H & 0xFF = 2
000A  03        OUTN
000B  00        HALT
```

```txt
ember> set 0x800 65
ember> set 0x801 66
ember> set 0x802 67
ember> set 0x803 0
ember> set 0 0x28
ember> set16 1 0x0800
ember> set 3 0x29
ember> set 4 0x02
ember> set 5 0x2B
ember> set 6 0x29
ember> set 7 0x02
ember> set 8 0x2B
ember> set 9 0x2D
ember> set 10 0x03
ember> set 11 0x00
ember> run
AB2 
halted after 10 steps
ember> regs
PC=0x000c  A=2  B=0  H=0x0802  Z=0 N=0 C=0
ember> quit
```

## Реалізовані рядки [ISA.uk.md](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md) (Лаба 3)

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x02` | `OUT` | 1 | — | надрукувати `A` як **символ** | 3 |
| `0x03` | `OUTN` | 1 | — | надрукувати `A` як **десяткове число** і пробіл | 3 |
| `0x22` | `LOAD A, [addr16]` | 3 | — | `A = mem[addr]` | 3 |
| `0x23` | `LOAD B, [addr16]` | 3 | — | `B = mem[addr]` | 3 |
| `0x24` | `STORE [addr16], A` | 3 | — | `mem[addr] = A` | 3 |
| `0x25` | `STORE [addr16], B` | 3 | — | `mem[addr] = B` | 3 |
| `0x26` | `MOV A, B` | 1 | — | `A = B` | 3 |
| `0x27` | `MOV B, A` | 1 | — | `B = A` | 3 |
| `0x28` | `LOADH H, imm16` | 3 | — | `H = imm16` — покласти адресу в адресний регістр | 3 |
| `0x29` | `LOAD A, [H]` | 1 | — | `A = mem[H]` — піти за вказівником | 3 |
| `0x2A` | `STORE [H], A` | 1 | — | `mem[H] = A` | 3 |
| `0x2B` | `INCH` | 1 | — | `H = H + 1` — крок до наступної комірки | 3 |
| `0x2C` | `DECH` | 1 | — | `H = H - 1` | 3 |
| `0x2D` | `HLOW` | 1 | — | *opt* `A = H & 0xFF` — молодший байт `H`, зручно надрукувати індекс | 4 |

`LOADI A/B` (`0x20`, `0x21`) — у таблиці Lab 02. `HLOW` в ISA позначений *opt* для Лаби 4; реалізований у Лабі 3 («Якщо встигаєте»).

Карта пам'яті (константи в `src/memory.hpp`):

| Регіон | Діапазон | Константа | З'являється в |
|---|---|---|---|
| Код | `0x000`–`0x7FF` | `CODE_LO`, `CODE_HI` | Lab 2 |
| Дані | `0x800`–`0x9FF` | `DATA_LO`, `DATA_HI` | Lab 3 |

Власних розширень у Лабі 3 немає; байт стану `0xB00` — як у Lab 02.

---

# Lab 04 — Форма керування

Нові й змінені команди REPL:

| Команда | Що робить |
|---|---|
| `run` | як у Lab 03, плюс дві нові зупинки: ліміт кроків (`step limit 100000 reached at …`) і точка зупину (`breakpoint at …`); після обох — `stopped after N steps`, машина не `halted`, наступний `run` іде далі |
| `b <addr>` / `b off` | точка зупину: `run` зупиняється перед інструкцією за адресою `addr` (0..4095); `b` або `b off` — зняти |
| `find <lo> <hi> <byte>` | перша адреса в `[lo, hi)`, де лежить `byte` (`0 <= lo <= hi <= 4096`), — даний цикл C++ |

Рядок трасування той самий, що в Lab 03: перша колонка — `PC` у момент вибірки (адреса інструкції), далі її байти, мнемоніка й `A B H Z N C` **після** виконання. Після стрибка, що спрацював, наступний рядок починається з адреси цілі.

## M1 — повний `switch`, невідомий опкод — помилка

`step()` (`src/cpu.cpp`) — два `switch`: зовнішній за групою (старший напівбайт, ISA §4), внутрішній за `Op`. Кожен відомий опкод має власний `case`, а `default` на обох рівнях повертає `StepResult::bad_opcode`. Першим невідомий опкод ловить `instr_size()` (`default: return 0;` — рядка в таблиці ISA немає), ще до перевірки `at + size > MEM_SIZE`; `default:` декодера — друга лінія з тією самою помилкою. «Зупиняє машину» означає: `step` відмовляє, `run` зупиняється, нічого не змінено (`PC`, регістри, прапорці, пам'ять); `halted` не ставиться, тож після виправлення байта машина працює далі.

`0xFF` — сміття, `0x1B` — рядок ISA Лаби 7 (`MUL`), ще не реалізований, `0x35` — у групі стрибків, але рядка немає:

```txt
ember> set 0 0xFF
ember> run
unknown opcode 0xff at 0x0000; nothing done
stopped after 0 steps
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> set 0 0x1B
ember> step
unknown opcode 0x1b at 0x0000; nothing done
ember> set 0 0x35
ember> step
unknown opcode 0x35 at 0x0000; nothing done
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> set 0 0x00
ember> run
halted after 1 steps
ember> quit
```

## M2 — `CMP`, `JMP`, `JZ`, `JNZ`

`CMP` — це `alu_sub(cpu.a, cpu.b, cpu.f)` без запису в `A`. `step` спершу рахує, де наступна інструкція; стрибок, що спрацював, **замінює** це значення на `addr16`, а не додає до нього:

```cpp
    std::uint16_t next = static_cast<std::uint16_t>(at + size);
    ...
        case Op::Jz:
            if (cpu.f.z && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
    ...
    cpu.pc = next;
```

```txt
адр.  байти       текст
0000  1A          CMP A, B
0001  31 10 00    JZ 0x0010
```

`A = B = 5`: `Z = 1`, стрибок спрацював — `PC = 0x0010`, а не `0x0013`:

```txt
ember> reg a 5
ember> reg b 5
ember> set 0 0x1A
ember> set 1 0x31
ember> set16 2 0x0010
ember> step
0000  1a        CMP A, B          -> A=  5  B=  5  H=0x0000  Z=1 N=0 C=0
ember> regs
PC=0x0001  A=5  B=5  H=0x0000  Z=1 N=0 C=0
ember> step
0001  31 10 00  JZ 0x0010         -> A=  5  B=  5  H=0x0000  Z=1 N=0 C=0
ember> regs
PC=0x0010  A=5  B=5  H=0x0000  Z=1 N=0 C=0
ember> quit
```

`A = 3`, `B = 5`: `3 - 5 = 254` з позикою (`Z=0 N=1 C=1`), стрибок не спрацював — `PC = 0x0004`:

```txt
ember> reg a 3
ember> reg b 5
ember> set 0 0x1A
ember> set 1 0x31
ember> set16 2 0x0010
ember> step
0000  1a        CMP A, B          -> A=  3  B=  5  H=0x0000  Z=0 N=1 C=1
ember> regs
PC=0x0001  A=3  B=5  H=0x0000  Z=0 N=1 C=1
ember> step
0001  31 10 00  JZ 0x0010         -> A=  3  B=  5  H=0x0000  Z=0 N=1 C=1
ember> regs
PC=0x0004  A=3  B=5  H=0x0000  Z=0 N=1 C=1
ember> quit
```

Операнд стрибка лежить молодшим байтом уперед і читається даним `get16`: `30 23 01` — це `JMP 0x0123`. Стрибок, що спрацював, на адресу `>= 4096` відхиляється на самому стрибку: повідомлення, нічого не змінено, `PC` лишається на `JMP`. Стрибок, що не спрацював, ціль не використовує — `PC += 3` (той самий принцип, що з `H` у Lab 03: тримати погану адресу можна, користуватись — ні). Прапорців стрибки не чіпають:

```txt
ember> set 0 0x30
ember> set16 1 0x0123
ember> dump 0 16
0000  30 23 01 00 00 00 00 00 00 00 00 00 00 00 00 00  |0#..............|
ember> step
0000  30 23 01  JMP 0x0123        -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> regs
PC=0x0123  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> reg pc 0
ember> set16 1 0x1000
ember> step
address 0x1000 is outside 0..4095 (JMP 0x1000 at 0x0000); nothing done
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> set 0 0x31
ember> step
0000  31 00 10  JZ 0x1000         -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> regs
PC=0x0003  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> quit
```

### Ліміт кроків у `run`

`src/run.hpp`:

```cpp
// A program that never reaches HALT (JMP to itself) must not hang the REPL.
// Counted per `run` command; the next `run` gets the same budget again.
const unsigned long RUN_STEP_LIMIT = 100000;
```

`src/run.cpp`:

```cpp
RunResult run(CPU& cpu, bool trace, long breakpoint) {
    unsigned long steps = 0;
    while (true) {
        if (steps > 0 && cpu.pc == breakpoint)
            return {RunStop::breakpoint, StepResult::ok, steps};
        if (steps == RUN_STEP_LIMIT)
            return {RunStop::step_limit, StepResult::ok, steps};
        const StepResult r = step(cpu, trace);
        if (r != StepResult::ok)
            return {RunStop::error, r, steps};
        ++steps;
        if (cpu.halted)
            return {RunStop::halted, StepResult::ok, steps};
    }
}
```

Ліміт — `RUN_STEP_LIMIT = 100000` кроків на **один** виклик `run`. Якщо за 100000 кроків не виконався `HALT`, `run` друкує `step limit 100000 reached at 0x....` (адреса наступної, ще не виконаної інструкції) і `stopped after 100000 steps`. Машина не `halted`: `step` і наступний `run` продовжують з того самого `PC`. Якщо `HALT` — рівно 100000-й крок, це звичайне `halted after 100000 steps`. `JMP` сам на себе (`30 00 00`):

```txt
ember> set 0 0x30
ember> run
step limit 100000 reached at 0x0000
stopped after 100000 steps
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> step
0000  30 00 00  JMP 0x0000        -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> quit
```

## M3 — цикл у байткоді

Зворотний відлік з [ISA.uk.md §9](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md#9-дві-програми-щоб-перевірити-себе):

```txt
адр.  байти       текст
0000  20 03       LOADI A, 3
0002  03          loop:  OUTN
0003  19                 DEC A
0004  32 02 00           JNZ loop
0007  00                 HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x03
ember> set 2 0x03
ember> set 3 0x19
ember> set 4 0x32
ember> set 5 0x02
ember> set 6 0x00
ember> set 7 0x00
ember> trace on
ember> run
0000  20 03     LOADI A, 0x03     -> A=  3  B=  0  H=0x0000  Z=0 N=0 C=0
3 
0002  03        OUTN              -> A=  3  B=  0  H=0x0000  Z=0 N=0 C=0
0003  19        DEC A             -> A=  2  B=  0  H=0x0000  Z=0 N=0 C=0
0004  32 02 00  JNZ 0x0002        -> A=  2  B=  0  H=0x0000  Z=0 N=0 C=0
2 
0002  03        OUTN              -> A=  2  B=  0  H=0x0000  Z=0 N=0 C=0
0003  19        DEC A             -> A=  1  B=  0  H=0x0000  Z=0 N=0 C=0
0004  32 02 00  JNZ 0x0002        -> A=  1  B=  0  H=0x0000  Z=0 N=0 C=0
1 
0002  03        OUTN              -> A=  1  B=  0  H=0x0000  Z=0 N=0 C=0
0003  19        DEC A             -> A=  0  B=  0  H=0x0000  Z=1 N=0 C=0
0004  32 02 00  JNZ 0x0002        -> A=  0  B=  0  H=0x0000  Z=1 N=0 C=0
0007  00        HALT              -> A=  0  B=  0  H=0x0000  Z=1 N=0 C=0
halted after 11 steps
ember> regs
PC=0x0008  A=0  B=0  H=0x0000  Z=1 N=0 C=0
ember> quit
```

11 рядків — по одному на крок (`3n + 2` для `n = 3`). Після `0004 … JNZ 0x0002` при `Z=0` наступний рядок — `0002`; на останньому `JNZ` `Z=1`, і `PC` іде далі на `0007`.

Те саме на C++ (тимчасовий `scratch.cpp` у корені проєкту, він у `.gitignore`; після прогону видалений):

```cpp
#include <cstdint>
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    const std::uint8_t n = static_cast<std::uint8_t>(argc > 1 ? std::atoi(argv[1]) : 3);

    std::uint8_t a = n;
    while (a != 0) { // перевірка ДО тіла: 0 разів, якщо a == 0
        std::cout << +a << ' ';
        a = a - 1;
    }
    std::cout << "| while\n";

    a = n;
    do { // перевірка ПІСЛЯ тіла, як JNZ унизу: щонайменше раз
        std::cout << +a << ' ';
        a = a - 1;
    } while (a != 0);
    std::cout << "| do-while\n";
}
```

```bash
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined scratch.cpp -o scratch && ./scratch 3 && ./scratch 0
```

```txt
3 2 1 | while
3 2 1 | do-while
| while
0 255 254 253 252 251 250 249 248 247 246 245 244 243 242 241 240 239 238 237 236 235 234 233 232 231 230 229 228 227 226 225 224 223 222 221 220 219 218 217 216 215 214 213 212 211 210 209 208 207 206 205 204 203 202 201 200 199 198 197 196 195 194 193 192 191 190 189 188 187 186 185 184 183 182 181 180 179 178 177 176 175 174 173 172 171 170 169 168 167 166 165 164 163 162 161 160 159 158 157 156 155 154 153 152 151 150 149 148 147 146 145 144 143 142 141 140 139 138 137 136 135 134 133 132 131 130 129 128 127 126 125 124 123 122 121 120 119 118 117 116 115 114 113 112 111 110 109 108 107 106 105 104 103 102 101 100 99 98 97 96 95 94 93 92 91 90 89 88 87 86 85 84 83 82 81 80 79 78 77 76 75 74 73 72 71 70 69 68 67 66 65 64 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 | do-while
```

Одна форма, два записи: `OUTN` — `std::cout << +a`, `DEC A` — `a = a - 1` і водночас умова (`Z` = `a == 0`), `JNZ loop` — `a != 0` і стрибок назад. `JNZ` стоїть **після** тіла, тому програма з ISA §9 — це `do-while`: для `a = 3` обидва цикли друкують `3 2 1`, а для `a = 0` `while` не друкує нічого, тоді як `do-while` — 256 чисел, як і `ember` з `20 00` (770 кроків). `while` з перевіркою **до** тіла в байткоді — `CMP`/`JZ` на вихід угорі циклу й `JMP` назад унизу.

## M4 — лінійний пошук, двічі

### `find` на C++

Даний цикл з теорії §3, `src/search.cpp` (`clang-format` переніс `return i;` на окремий рядок, більше нічого не змінено):

```cpp
// знайти перше i в [lo, hi), де mem_get(mem, i) == needle; якщо промах — повернути hi
std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) == needle)
            return i;
    }
    return hi;
}
```

`hi` не входить у ділянку; `lo == hi` — порожня ділянка, тобто промах; `hi = 4096` — до кінця коробки:

```txt
ember> set 0x803 0x41
ember> set 0x805 0x41
ember> find 0x800 0x808 0x41
found 0x41 at 0x0803
ember> find 0x800 0x803 0x41
0x41 not found in [0x0800, 0x0803)
ember> set 0x808 0x41
ember> find 0x804 0x808 0x41
found 0x41 at 0x0805
ember> find 0x806 0x808 0x41
0x41 not found in [0x0806, 0x0808)
ember> find 0x806 0x809 0x41
found 0x41 at 0x0808
ember> find 0x808 0x808 0x41
0x41 not found in [0x0808, 0x0808)
ember> find 0x808 0x800 0x41
range must be 0 <= lo <= hi <= 4096
ember> find 0 4097 0
range must be 0 <= lo <= hi <= 4096
ember> find 0 8 300
a byte is 0..255, got 300
ember> find 0x800
usage: find <lo> <hi> <byte>
ember> quit
```

### Пошук усередині `ember`

Шукає `0x41` у `[0x800, 0x808)`, друкує індекс або `-`:

```txt
адр.  байти       текст
0000  28 00 08          LOADH H, 0x0800   ; i = lo
0003  21 41             LOADI B, 0x41     ; що шукаємо; B більше не змінюється
0005  29         loop:  LOAD  A, [H]      ; A = mem[i]
0006  1A                CMP   A, B        ; Z = (mem[i] == 0x41)
0007  31 16 00          JZ    found
000A  2B                INCH              ; ++i
000B  2D                HLOW              ; A = i (молодший байт H)
000C  17                SHR   A
000D  17                SHR   A
000E  17                SHR   A           ; A = i / 8: 0, поки i < 8
000F  31 05 00          JZ    loop        ; ще в ділянці
0012  20 2D             LOADI A, '-'      ; промах
0014  02                OUT
0015  00                HALT
0016  2D         found: HLOW              ; A = індекс
0017  03                OUTN
0018  00                HALT
```

**Як цикл дізнається, що дійшов до `hi`.** `B` увесь цикл тримає `0x41`, тому кінець перевіряється без `CMP`. Після `INCH` `HLOW` кладе в `A` молодший байт `H`; для `lo = 0x800` це індекс наступної комірки, 1..8. Три `SHR A` ділять його на 8: поки `i < 8`, виходить 0, `Z = 1`, і `JZ loop` повертає на початок. На `i = 8` (`H = 0x808 = hi`) виходить 1, `Z = 0`, і цикл закінчується, не прочитавши `mem[0x808]`.

**Як він повідомляє про промах.** Знайдений індекс друкує `OUTN` — число з пробілом (`0 `…`7 `); промах — символ `-` через `OUT`, який з індексом не сплутати.

Обмеження: трюк працює лише тому, що `lo = 0x800` вирівняний на 8 (молодший байт `H` і є індексом), а довжина 8 — степінь двійки. Для довільної ділянки кінець треба порівнювати через `CMP` з `hi` у `B` і на кожному колі заново класти `0x41` у `B`. Кроків: збіг на індексі `k` — `8 + 9k`, промах — 77.

Знайдено (`0x41` на індексах 1 і 3 — друкується перше входження), покроково; поруч той самий пошук командою `find`:

```txt
ember> set 0x00 0x28
ember> set16 0x01 0x0800
ember> set 0x03 0x21
ember> set 0x04 0x41
ember> set 0x05 0x29
ember> set 0x06 0x1A
ember> set 0x07 0x31
ember> set16 0x08 0x0016
ember> set 0x0A 0x2B
ember> set 0x0B 0x2D
ember> set 0x0C 0x17
ember> set 0x0D 0x17
ember> set 0x0E 0x17
ember> set 0x0F 0x31
ember> set16 0x10 0x0005
ember> set 0x12 0x20
ember> set 0x13 0x2D
ember> set 0x14 0x02
ember> set 0x15 0x00
ember> set 0x16 0x2D
ember> set 0x17 0x03
ember> set 0x18 0x00
ember> set 0x800 0x10
ember> set 0x801 0x41
ember> set 0x802 0x30
ember> set 0x803 0x41
ember> dump 0 32
0000  28 00 08 21 41 29 1a 31 16 00 2b 2d 17 17 17 31  |(..!A).1..+-...1|
0010  05 00 20 2d 02 00 2d 03 00 00 00 00 00 00 00 00  |.. -..-.........|
ember> dump 0x800 16
0800  10 41 30 41 00 00 00 00 00 00 00 00 00 00 00 00  |.A0A............|
ember> trace on
ember> run
0000  28 00 08  LOADH H, 0x0800   -> A=  0  B=  0  H=0x0800  Z=0 N=0 C=0
0003  21 41     LOADI B, 0x41     -> A=  0  B= 65  H=0x0800  Z=0 N=0 C=0
0005  29        LOAD A, [H]       -> A= 16  B= 65  H=0x0800  Z=0 N=0 C=0
0006  1a        CMP A, B          -> A= 16  B= 65  H=0x0800  Z=0 N=1 C=1
0007  31 16 00  JZ 0x0016         -> A= 16  B= 65  H=0x0800  Z=0 N=1 C=1
000a  2b        INCH              -> A= 16  B= 65  H=0x0801  Z=0 N=1 C=1
000b  2d        HLOW              -> A=  1  B= 65  H=0x0801  Z=0 N=1 C=1
000c  17        SHR A             -> A=  0  B= 65  H=0x0801  Z=1 N=0 C=1
000d  17        SHR A             -> A=  0  B= 65  H=0x0801  Z=1 N=0 C=0
000e  17        SHR A             -> A=  0  B= 65  H=0x0801  Z=1 N=0 C=0
000f  31 05 00  JZ 0x0005         -> A=  0  B= 65  H=0x0801  Z=1 N=0 C=0
0005  29        LOAD A, [H]       -> A= 65  B= 65  H=0x0801  Z=1 N=0 C=0
0006  1a        CMP A, B          -> A= 65  B= 65  H=0x0801  Z=1 N=0 C=0
0007  31 16 00  JZ 0x0016         -> A= 65  B= 65  H=0x0801  Z=1 N=0 C=0
0016  2d        HLOW              -> A=  1  B= 65  H=0x0801  Z=1 N=0 C=0
1 
0017  03        OUTN              -> A=  1  B= 65  H=0x0801  Z=1 N=0 C=0
0018  00        HALT              -> A=  1  B= 65  H=0x0801  Z=1 N=0 C=0
halted after 17 steps
ember> regs
PC=0x0019  A=1  B=65  H=0x0801  Z=1 N=0 C=0
ember> find 0x800 0x808 0x41
found 0x41 at 0x0801
ember> quit
```

Промах: `0x41` лежить на `0x808`, тобто вже за `hi`:

```txt
ember> set 0x00 0x28
ember> set16 0x01 0x0800
ember> set 0x03 0x21
ember> set 0x04 0x41
ember> set 0x05 0x29
ember> set 0x06 0x1A
ember> set 0x07 0x31
ember> set16 0x08 0x0016
ember> set 0x0A 0x2B
ember> set 0x0B 0x2D
ember> set 0x0C 0x17
ember> set 0x0D 0x17
ember> set 0x0E 0x17
ember> set 0x0F 0x31
ember> set16 0x10 0x0005
ember> set 0x12 0x20
ember> set 0x13 0x2D
ember> set 0x14 0x02
ember> set 0x15 0x00
ember> set 0x16 0x2D
ember> set 0x17 0x03
ember> set 0x18 0x00
ember> set 0x800 1
ember> set 0x801 2
ember> set 0x802 3
ember> set 0x803 4
ember> set 0x804 5
ember> set 0x805 6
ember> set 0x806 7
ember> set 0x807 8
ember> set 0x808 0x41
ember> dump 0x800 16
0800  01 02 03 04 05 06 07 08 41 00 00 00 00 00 00 00  |........A.......|
ember> run
-
halted after 77 steps
ember> regs
PC=0x0016  A=45  B=65  H=0x0808  Z=0 N=0 C=0
ember> find 0x800 0x808 0x41
0x41 not found in [0x0800, 0x0808)
ember> find 0x800 0x809 0x41
found 0x41 at 0x0808
ember> quit
```

## Advanced

### `JC` / `JNC`

Рядки ISA `0x33`/`0x34` (*opt*): стрибок за прапорцем `C`. Після `CMP A, B` `C` — це позика, тобто `A < B` як беззнакові числа. Програма порівнює `A` і `B` двома способами:

```txt
адр.  байти       текст
0000  1A          CMP A, B
0001  33 10 00    JC 0x0010       ; A < B беззнаково?
0004  71 20 00    JL 0x0020       ; A < B знаково?
0007  00          HALT
...
0010  00          HALT            ; сюди — після JC
...
0020  00          HALT            ; сюди — після JL
```

`A = 1`, `B = 0x80`: `1 - 128` позичає, `C = 1`, `JC` спрацював:

```txt
ember> reg a 1
ember> reg b 0x80
ember> set 0x00 0x1A
ember> set 0x01 0x33
ember> set16 0x02 0x0010
ember> set 0x04 0x71
ember> set16 0x05 0x0020
ember> set 0x07 0x00
ember> set 0x10 0x00
ember> set 0x20 0x00
ember> trace on
ember> run
0000  1a        CMP A, B          -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
0001  33 10 00  JC 0x0010         -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
0010  00        HALT              -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
halted after 3 steps
ember> regs
PC=0x0011  A=1  B=128  H=0x0000  Z=0 N=1 C=1
ember> quit
```

### `JG` / `JL` — власні розширення

Визначення: `JG addr16` (`0x70`) стрибає, якщо `A > B`, а `JL addr16` (`0x71`) — якщо `A < B`, де обидва байти читаються як знакові числа в доповняльному коді, −128..127 (`0x80`..`0xFF` — це −128..−1). Обидві інструкції порівнюють `A` і `B` самі; прапорці не читають і не змінюють. Байт у знакове число перетворюю явно: `static_cast<std::int8_t>(200)` у C++17 залежить від реалізації (визначеним це стало лише в C++20):

```cpp
// A byte read as two's complement, -128..127, for JG/JL. Spelled out because
// static_cast<std::int8_t>(200) is implementation-defined before C++20.
static int as_signed(Byte v) { return v < 128 ? v : v - 256; }
```

```cpp
        case Op::Jl:
            if (as_signed(cpu.a) < as_signed(cpu.b) && !jump_to(w16, next))
                return StepResult::bad_address;
            break;
```

Чому не «знак за `N` після `CMP`»: `N` — старший біт 8-бітного результату `A - B`, а знакова різниця може не влізти в −128..127:

```txt
ember> alu sub 128 1
result=127  Z=0 N=0 C=0
ember> alu sub 127 255
result=128  Z=0 N=1 C=1
ember> quit
```

- `A = 0x80` (−128), `B = 1`: −128 − 1 = −129, у байт не влазить, результат `0x7F` = 127, `N = 0` — «за `N`» `A >= B`, а насправді −128 < 1.
- `A = 0x7F` (127), `B = 0xFF` (−1): 127 − (−1) = 128, результат `0x80`, `N = 1` — «за `N`» `A < B`, а насправді 127 > −1.

Тому справжні процесори мають прапорець переповнення `V` (`OF` на x86): він виставляється, коли знаковий результат не вліз — у віднімання це буває, коли `A` і `B` різних знаків, а знак результату не збігається зі знаком `A`. Знакове «менше» після `CMP` — `N ≠ V` (x86 `JL`: `SF ≠ OF`; ARM `LT`: `N ≠ V`), «більше» — `Z = 0` і `N = V`. В `ember` прапорця `V` немає, тому `JG`/`JL` порівнюють `A` і `B` самі. Цим вони відрізняються від x86 `JG`/`JL`, які читають прапорці, залишені попереднім `cmp`.

Перший контрприклад у ділі (та сама програма, що вище): `CMP` дає `N=0 C=0`, `JC` не спрацював (128 ≥ 1 беззнаково), `JL` спрацював (−128 < 1):

```txt
ember> reg a 0x80
ember> reg b 1
ember> set 0x00 0x1A
ember> set 0x01 0x33
ember> set16 0x02 0x0010
ember> set 0x04 0x71
ember> set16 0x05 0x0020
ember> set 0x07 0x00
ember> set 0x10 0x00
ember> set 0x20 0x00
ember> trace on
ember> run
0000  1a        CMP A, B          -> A=128  B=  1  H=0x0000  Z=0 N=0 C=0
0001  33 10 00  JC 0x0010         -> A=128  B=  1  H=0x0000  Z=0 N=0 C=0
0004  71 20 00  JL 0x0020         -> A=128  B=  1  H=0x0000  Z=0 N=0 C=0
0020  00        HALT              -> A=128  B=  1  H=0x0000  Z=0 N=0 C=0
halted after 4 steps
ember> regs
PC=0x0021  A=128  B=1  H=0x0000  Z=0 N=0 C=0
ember> quit
```

`JNC` і `JG`:

```txt
адр.  байти       текст
0000  1A          CMP A, B
0001  34 10 00    JNC 0x0010      ; A >= B беззнаково?
0004  70 20 00    JG 0x0020       ; A > B знаково?
0007  00          HALT
...
0010  00          HALT
...
0020  00          HALT
```

`A = 1`, `B = 0x80`: беззнаково `1 < 128`, `C = 1` — `JNC` не спрацював; знаково `1 > −128` — `JG` спрацював, хоча `N = 1`:

```txt
ember> reg a 1
ember> reg b 0x80
ember> set 0x00 0x1A
ember> set 0x01 0x34
ember> set16 0x02 0x0010
ember> set 0x04 0x70
ember> set16 0x05 0x0020
ember> set 0x07 0x00
ember> set 0x10 0x00
ember> set 0x20 0x00
ember> trace on
ember> run
0000  1a        CMP A, B          -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
0001  34 10 00  JNC 0x0010        -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
0004  70 20 00  JG 0x0020         -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
0020  00        HALT              -> A=  1  B=128  H=0x0000  Z=0 N=1 C=1
halted after 4 steps
ember> regs
PC=0x0021  A=1  B=128  H=0x0000  Z=0 N=1 C=1
ember> quit
```

### Точка зупину `b <addr>`

Одна точка; її адреса — змінна в `main()`, як `trace`. `run` перевіряє `PC == addr` перед кожним кроком, крім першого кроку цього `run`: інакше повторний `run` стояв би на місці (як `continue` у відладчику). `step` точку не враховує. Зворотний відлік з точкою на `JNZ`:

```txt
ember> set 0 0x20
ember> set 1 0x03
ember> set 2 0x03
ember> set 3 0x19
ember> set 4 0x32
ember> set 5 0x02
ember> set 6 0x00
ember> set 7 0x00
ember> b 4096
address 4096 is outside 0..4095
ember> b 4
breakpoint at 0x0004
ember> run
3 
breakpoint at 0x0004
stopped after 3 steps
ember> regs
PC=0x0004  A=2  B=0  H=0x0000  Z=0 N=0 C=0
ember> run
2 
breakpoint at 0x0004
stopped after 3 steps
ember> b off
no breakpoint
ember> run
1 
halted after 5 steps
ember> regs
PC=0x0008  A=0  B=0  H=0x0000  Z=1 N=0 C=0
ember> quit
```

### Godbolt — вкладений цикл

```cpp
int count(int n) {
    int s = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            s += j;
        }
    }
    return s;
}
```

Compiler Explorer, компілятор `g162` (x86-64 gcc 16.2), `-O0`, фільтри `intel`, `directives`, `labels`, `commentOnly`, `demangle` — запит через API:

`body.json`:

```json
{
  "source": "int count(int n) {\n    int s = 0;\n    for (int i = 0; i < n; ++i) {\n        for (int j = 0; j < i; ++j) {\n            s += j;\n        }\n    }\n    return s;\n}\n",
  "options": {
    "userArguments": "-O0",
    "filters": {"intel": true, "directives": true, "labels": true, "commentOnly": true, "demangle": true}
  }
}
```

```bash
curl -s -X POST -H 'Content-Type: application/json' -H 'Accept: text/plain' \
     --data @body.json https://godbolt.org/api/compiler/g162/compile
```

```txt
# Compilation provided by Compiler Explorer at https://godbolt.org/
"count(int)":
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-20], edi
        mov     DWORD PTR [rbp-4], 0
        mov     DWORD PTR [rbp-8], 0
        jmp     .L2
.L5:
        mov     DWORD PTR [rbp-12], 0
        jmp     .L3
.L4:
        mov     eax, DWORD PTR [rbp-12]
        add     DWORD PTR [rbp-4], eax
        add     DWORD PTR [rbp-12], 1
.L3:
        mov     eax, DWORD PTR [rbp-12]
        cmp     eax, DWORD PTR [rbp-8]
        jl      .L4
        add     DWORD PTR [rbp-8], 1
.L2:
        mov     eax, DWORD PTR [rbp-8]
        cmp     eax, DWORD PTR [rbp-20]
        jl      .L5
        mov     eax, DWORD PTR [rbp-4]
        pop     rbp
        ret
```

| Інструкції | Частина циклу |
|---|---|
| `mov DWORD PTR [rbp-8], 0` · `jmp .L2` | `i = 0`, одразу на перевірку `i < n` унизу |
| `.L5:` `mov DWORD PTR [rbp-12], 0` · `jmp .L3` | тіло зовнішнього циклу: `j = 0`, одразу на перевірку `j < i` |
| `.L4:` `mov` · `add DWORD PTR [rbp-4], eax` · `add DWORD PTR [rbp-12], 1` | `s += j`, `++j` |
| `.L3:` `cmp eax, DWORD PTR [rbp-8]` · `jl .L4` | `j < i` — назад на тіло внутрішнього |
| `add DWORD PTR [rbp-8], 1` | `++i` |
| `.L2:` `cmp eax, DWORD PTR [rbp-20]` · `jl .L5` | `i < n` — назад на тіло зовнішнього |

Обидва `for` зібрані так само, як наш відлік: перевірка внизу і умовний стрибок назад (`jl` ↔ `JNZ loop`), а `jmp` перед циклом (↔ наш `JMP`) перескакує на перевірку, щоб при `n <= 0` тіло не виконалось жодного разу — так `for` стає `do-while` з вхідним стрибком. `cmp` ↔ наш `CMP`. `jl` — знакове «менше» для `int`, тобто наш `JL`, тільки x86 бере його з прапорців `cmp` (`SF ≠ OF`).

## Якщо встигаєте

`JG`/`JL` зі знаковим порівнянням (і чому потрібен прапорець переповнення), вкладений цикл у Godbolt і `b <addr>` — у розділі Advanced вище. `HLOW` (ISA *opt*, Лаба 4) реалізований у Лабі 3 і стоїть у її таблиці; тут ним користується програма пошуку.

## Реалізовані рядки [ISA.uk.md](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md) (Лаба 4)

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x1A` | `CMP A, B` | 1 | Z N C | порахувати `A - B`, виставити прапорці, **результат викинути** | 4 |
| `0x30` | `JMP addr16` | 3 | — | стрибнути завжди | 4 |
| `0x31` | `JZ addr16` | 3 | — | стрибнути, якщо `Z` виставлений | 4 |
| `0x32` | `JNZ addr16` | 3 | — | стрибнути, якщо `Z` не виставлений | 4 |
| `0x33` | `JC addr16` | 3 | — | *opt* стрибнути, якщо `C` виставлений | 4 |
| `0x34` | `JNC addr16` | 3 | — | *opt* стрибнути, якщо `C` не виставлений | 4 |

Стрибок, що спрацював: `PC = addr16`, розмір не додається; ціль `>= 4096` — помилка на самому стрибку.

## Власні розширення

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x70` | `JG addr16` | 3 | — | стрибнути, якщо `A > B` як знакові числа (−128..127); прапорці не читає й не змінює | 4 |
| `0x71` | `JL addr16` | 3 | — | стрибнути, якщо `A < B` як знакові числа (−128..127); прапорці не читає й не змінює | 4 |

Номери — з вільного діапазону `0x70`–`0xFF` (ISA §8); жоден рядок ISA не перевизначений. Байт стану `0xB00` — як у Lab 02, без змін.

---

# Lab 05 — Багато однакового: масиви, рядки, пошук і екран

Нові й змінені команди REPL:

| Команда | Що робить |
|---|---|
| `show` | намалювати екран: даний `show()`, 32 рядки по 64 символи, `#` — піксель горить, `.` — ні |
| `cls` | `clear()`: 256 нулів на `0xA00`–`0xAFF`, без виводу |
| `plot <x> <y>` | засвітити піксель, `x` 0..63, `y` 0..31; за екраном — `pixel (64, 0) is outside the 64x32 screen; nothing changed` |
| `unplot <x> <y>` | погасити піксель; помилки як у `plot` |
| `frame` | рамка по краю екрана через `plot`; екран не очищає |
| `rowmin <y>` | найлівіший засвічений піксель у рядку `y` (0..31) |
| `find <lo> <hi> !<byte>` | перша адреса в `[lo, hi)`, де лежить **не** `byte`; `find <lo> <hi> <byte>` — як у Lab 04 |
| `sort <lo> <hi> [bubble\|insert] [show]` | відсортувати байти `[lo, hi)` на місці (за замовчуванням бульбашкою); `show` — стовпчики на екрані після кожного проходу |
| `anim <steps> <frames>` | `frames` разів: `run` щонайбільше `steps` кроків, потім `show` |

Рядок трасування той самий, що в Lab 03/04; нові мнемоніки — `OUTS 0x0800`, `CLS`, `PLOT`, `UNPLOT`, `SHOW`. `SHOW` — вивід гостя: екран друкується і в тихому `run`.

## M1 — відеопам'ять

Екран — це 256 байтів усередині `Memory`, `mem.data[0xA00..0xAFF]` (`VRAM_LO`/`VRAM_HI` у `src/memory.hpp`); окремого масиву пікселів на хості немає. Піксель `(x, y)` — один біт ([ISA.uk.md §7](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md#7-екран-це-память)):

```txt
адреса байта = VRAM_LO + y * 8 + (x / 8)
номер біта   = 7 - (x % 8)            ; біт 7 — найлівіший піксель у байті
горить       = (mem[адреса байта] >> номер біта) & 1
```

`show()` дано, він вставлений у `src/display.cpp` дослівно; темний піксель там — `.`, а не пробіл, як у контракті екрана, — лишаю як у даному коді. `plot` і `clear` написані за тією самою формулою, лише через `mem_get`/`mem_set`. Прототип лаби — `void plot(...)`; у мене `plot` повертає `bool`: `false`, якщо піксель за екраном (від'ємні `x`, `y` теж), і тоді нічого не записано. Так `PLOT` може виставити `C`, а REPL — надрукувати повідомлення, як із `mem_set`:

```cpp
static bool on_screen(int x, int y) { return x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H; }

bool plot(Memory& mem, int x, int y) {
    if (!on_screen(x, y))
        return false; // checked before the write: a refused plot changes nothing
    const std::size_t addr = VRAM_LO + y * 8 + x / 8;
    const Byte mask = static_cast<Byte>(1u << (7 - x % 8));
    mem_set(mem, addr, static_cast<Byte>(mem_get(mem, addr) | mask)); // the other 7 pixels stay
    return true;
}

void clear(Memory& mem) {
    for (std::size_t i = 0; i < 256; ++i)
        mem_set(mem, VRAM_LO + i, 0);
}
```

Відхилені координати нічого не змінюють — ні `0xA08` (куди впав би `(64, 0)`), ні `0xB00` (куди впав би `(0, 32)`):

```txt
ember> plot 64 0
pixel (64, 0) is outside the 64x32 screen; nothing changed
ember> plot 0 32
pixel (0, 32) is outside the 64x32 screen; nothing changed
ember> plot -1 0
pixel (-1, 0) is outside the 64x32 screen; nothing changed
ember> plot 1
usage: plot <x> <y>
ember> dump 0xA00 16
0a00  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> get 2816
0  0x00  0b00000000  '.'
ember> quit
```

`clear` пише рівно 256 нулів: `0x9FF` перед екраном і `0xB00` після нього лишаються:

```txt
ember> set 0x9FF 1
ember> set 0xB00 2
ember> frame
ember> cls
ember> dump 0x9F0 288
09f0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01  |................|
0a00  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a10  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a20  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a30  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a40  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a50  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a60  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a70  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a80  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0a90  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0aa0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0ab0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0ac0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0ad0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0ae0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0af0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0b00  02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> quit
```

Рамка з C++ — команда `frame` (`plot` по краях):

```txt
ember> cls
ember> frame
ember> show
################################################################
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
#..............................................................#
################################################################
ember> quit
```

## M2 — `CLS` / `PLOT` / `SHOW` як інструкції

У `step()` — нова група `case 0x4:` із власним `switch` за `Op` і `default` → невідомий опкод. `PLOT`: `x = A`, `y = B`, той самий `plot()`, що в REPL; за екраном — `C = 1` і нічого не змінено, на екрані — `C = 0`; `Z`, `N` не чіпає, `PC += 1` в обох випадках (це не помилка):

```cpp
    case 0x4:
        // The screen. PLOT/UNPLOT touch only C: 1 = off the screen and
        // nothing changed, 0 = done. Z and N are never touched.
        switch (static_cast<Op>(op)) {
        case Op::Cls:
            clear(*cpu.mem);
            break;
        case Op::Plot:
            cpu.f.c = !plot(*cpu.mem, cpu.a, cpu.b);
            break;
        case Op::Unplot:
            cpu.f.c = !unplot(*cpu.mem, cpu.a, cpu.b);
            break;
        case Op::Show:
            guest_end_line(); // the screen starts on a line of its own
            show(*cpu.mem);
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;
```

Три пікселі:

```txt
адр.  байти       текст
0000  40          CLS
0001  20 14       LOADI A, 20
0003  21 02       LOADI B, 2
0005  41          PLOT              ; (20, 2) -> 0xA12, біт 3 -> 0x08
0006  20 0A       LOADI A, 10
0008  21 03       LOADI B, 3
000A  41          PLOT              ; (10, 3) -> 0xA19, біт 5 -> 0x20
000B  20 3F       LOADI A, 63
000D  21 1F       LOADI B, 31
000F  41          PLOT              ; (63, 31) -> 0xAFF, біт 0 -> 0x01
0010  00          HALT
```

```txt
ember> set 0 0x40
ember> set 1 0x20
ember> set 2 0x14
ember> set 3 0x21
ember> set 4 0x02
ember> set 5 0x41
ember> set 6 0x20
ember> set 7 0x0A
ember> set 8 0x21
ember> set 9 0x03
ember> set 10 0x41
ember> set 11 0x20
ember> set 12 0x3F
ember> set 13 0x21
ember> set 14 0x1F
ember> set 15 0x41
ember> set 16 0x00
ember> trace on
ember> run
0000  40        CLS               -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
0001  20 14     LOADI A, 0x14     -> A= 20  B=  0  H=0x0000  Z=0 N=0 C=0
0003  21 02     LOADI B, 0x02     -> A= 20  B=  2  H=0x0000  Z=0 N=0 C=0
0005  41        PLOT              -> A= 20  B=  2  H=0x0000  Z=0 N=0 C=0
0006  20 0a     LOADI A, 0x0a     -> A= 10  B=  2  H=0x0000  Z=0 N=0 C=0
0008  21 03     LOADI B, 0x03     -> A= 10  B=  3  H=0x0000  Z=0 N=0 C=0
000a  41        PLOT              -> A= 10  B=  3  H=0x0000  Z=0 N=0 C=0
000b  20 3f     LOADI A, 0x3f     -> A= 63  B=  3  H=0x0000  Z=0 N=0 C=0
000d  21 1f     LOADI B, 0x1f     -> A= 63  B= 31  H=0x0000  Z=0 N=0 C=0
000f  41        PLOT              -> A= 63  B= 31  H=0x0000  Z=0 N=0 C=0
0010  00        HALT              -> A= 63  B= 31  H=0x0000  Z=0 N=0 C=0
halted after 11 steps
ember> show
................................................................
................................................................
....................#...........................................
..........#.....................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
...............................................................#
ember> quit
```

Піксель за екраном: `C = 1` (і `Z = 1` від `CMP` не змінився), наступний `PLOT` на екрані — знову `C = 0`; у відеопам'яті лише `(5, 0)`:

```txt
адр.  байти       текст
0000  1A          CMP A, B          ; A = B = 0: Z = 1
0001  20 40       LOADI A, 64
0003  21 00       LOADI B, 0
0005  41          PLOT              ; (64, 0): за екраном -> C = 1
0006  20 05       LOADI A, 5
0008  41          PLOT              ; (5, 0) -> 0xA00, біт 2 -> 0x04, C = 0
0009  00          HALT
```

```txt
ember> set 0 0x1A
ember> set 1 0x20
ember> set 2 64
ember> set 3 0x21
ember> set 4 0
ember> set 5 0x41
ember> set 6 0x20
ember> set 7 5
ember> set 8 0x41
ember> set 9 0x00
ember> trace on
ember> run
0000  1a        CMP A, B          -> A=  0  B=  0  H=0x0000  Z=1 N=0 C=0
0001  20 40     LOADI A, 0x40     -> A= 64  B=  0  H=0x0000  Z=1 N=0 C=0
0003  21 00     LOADI B, 0x00     -> A= 64  B=  0  H=0x0000  Z=1 N=0 C=0
0005  41        PLOT              -> A= 64  B=  0  H=0x0000  Z=1 N=0 C=1
0006  20 05     LOADI A, 0x05     -> A=  5  B=  0  H=0x0000  Z=1 N=0 C=1
0008  41        PLOT              -> A=  5  B=  0  H=0x0000  Z=1 N=0 C=0
0009  00        HALT              -> A=  5  B=  0  H=0x0000  Z=1 N=0 C=0
halted after 7 steps
ember> regs
PC=0x000a  A=5  B=0  H=0x0000  Z=1 N=0 C=0
ember> dump 0xA00 16
0a00  04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> quit
```

## M3 — екран це пам'ять

Друга програма з [ISA.uk.md §9](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md#9-дві-програми-щоб-перевірити-себе) — піксель без `PLOT`:

```txt
адр.  байти       текст
0000  40          CLS
0001  20 80       LOADI A, 0x80
0003  28 00 0A    LOADH H, 0x0A00
0006  2A          STORE [H], A
0007  43          SHOW
0008  00          HALT
```

Це `checks/lab-05.txt` плюс `dump 0xA00 16`. Екран друкує сам `SHOW`, у куті рівно один `#`:

```txt
ember> set 0 0x40
ember> set 1 0x20
ember> set 2 0x80
ember> set 3 0x28
ember> set 4 0x00
ember> set 5 0x0A
ember> set 6 0x2A
ember> set 7 0x43
ember> set 8 0x00
ember> run
#...............................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
halted after 6 steps
ember> get 2560
128  0x80  0b10000000  '.'
ember> dump 0xA00 16
0a00  80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> quit
```

Крок рядка: `plot 0 1` кладе `0x80` на `0xA08` = `0xA00 + 1 * 8`, піксель — на початку **другого** рядка. `plot 0 0` дає той самий `0x80` на `0xA00`, що й `STORE [H], A`:

```txt
ember> plot 0 1
ember> get 2568
128  0x80  0b10000000  '.'
ember> show
................................................................
#...............................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
ember> cls
ember> plot 0 0
ember> get 2560
128  0x80  0b10000000  '.'
ember> quit
```

Рамка з M1 у пам'яті:

```txt
ember> cls
ember> frame
ember> dump 0xA00 256
0a00  ff ff ff ff ff ff ff ff 80 00 00 00 00 00 00 01  |................|
0a10  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a20  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a30  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a40  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a50  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a60  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a70  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a80  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0a90  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0aa0  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0ab0  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0ac0  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0ad0  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0ae0  80 00 00 00 00 00 00 01 80 00 00 00 00 00 00 01  |................|
0af0  80 00 00 00 00 00 00 01 ff ff ff ff ff ff ff ff  |................|
ember> quit
```

Рядок дампу — 16 байтів, тобто два рядки екрана по 8 байтів: `0a00` — рядки `y = 0` і `y = 1`, `0a10` — `y = 2` і `y = 3` і так далі. Байт `0xA08 = 0x80 = 10000000`: `0xA08 - 0xA00 = 8 = 1 * 8 + 0`, отже це рядок `y = 1`, пікселі `x = 0..7`; біт 7 — найлівіший, тож горить лише `(0, 1)` — ліва стінка рамки, а `(1, 1)`…`(7, 1)` темні. Останній байт того самого рядка, `0xA0F = 0x01`, — пікселі `(56..63, 1)`, горить тільки біт 0, тобто `(63, 1)` — права стінка. Вісім `ff` на `0xA00`–`0xA07` — увесь верхній рядок.

## M4 — рядки: `OUTS`

`OUTS addr16` друкує байти гостя до `0` (сам `0` не друкується), але не більше 256 (`OUTS_MAX`) і не далі кінця коробки: `0xFFF` — ще одна стеля, не помилка. Помилка — лише операнд `>= 4096`, і тоді нічого не надруковано. Байти читає `guest_read` (той самий шлях, що й `LOAD`), друкує `guest_putc` (як `OUT`); прапорці не змінюються:

```cpp
        case Op::Outs:
            // A C string of the guest: bytes up to the 0 (not printed), but at
            // most OUTS_MAX of them, and never past the box: the end of the box
            // is one more ceiling, not an error. Only a start outside the box
            // is refused, before anything is printed.
            if (w16 >= MEM_SIZE)
                return StepResult::bad_address;
            for (std::size_t addr = w16, n = 0; n < OUTS_MAX && guest_read(cpu, addr, v) && v != 0;
                 ++addr, ++n)
                guest_putc(v);
            break;
```

Рядок гостя — це байти в пам'яті `ember` і цикл по них; `std::string` тут немає. На хості `std::string` є — для тексту мнемоніки в трасуванні.

`HELLO\0` на `0x800`:

```txt
адр.  байти       текст
0000  04 00 08    OUTS 0x0800
0003  00          HALT
```

```txt
ember> set 0x800 72
ember> set 0x801 69
ember> set 0x802 76
ember> set 0x803 76
ember> set 0x804 79
ember> set 0x805 0
ember> set 0 0x04
ember> set 1 0x00
ember> set 2 0x08
ember> set 3 0x00
ember> run
HELLO
halted after 2 steps
ember> quit
```

`OUTS` з середини рядка й з адреси, де вже `0` (`step` друкує рядок трасування після виводу кожної інструкції):

```txt
адр.  байти       текст
0000  04 00 08    OUTS 0x0800       ; HELLO
0003  04 02 08    OUTS 0x0802       ; LLO
0006  04 05 08    OUTS 0x0805       ; там уже 0: нічого
0009  00          HALT
```

```txt
ember> set 0x800 72
ember> set 0x801 69
ember> set 0x802 76
ember> set 0x803 76
ember> set 0x804 79
ember> set 0x805 0
ember> set 0 0x04
ember> set 1 0x00
ember> set 2 0x08
ember> set 3 0x04
ember> set 4 0x02
ember> set 5 0x08
ember> set 6 0x04
ember> set 7 0x05
ember> set 8 0x08
ember> set 9 0x00
ember> step
HELLO
0000  04 00 08  OUTS 0x0800       -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> step
LLO
0003  04 02 08  OUTS 0x0802       -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> step
0006  04 05 08  OUTS 0x0805       -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> step
0009  00        HALT              -> A=  0  B=  0  H=0x0000  Z=0 N=0 C=0
ember> quit
```

Стеля: цикл гостя заповнює весь регіон даних `0x800`–`0x9FF` — 512 байтів `A` без жодного нуля, потім `OUTS 0x0800`:

```txt
адр.  байти       текст
0000  28 00 08    LOADH H, 0x0800   ; H = 0x800
0003  21 02       LOADI B, 2        ; B = 2 кола по 256
0005  20 41       LOADI A, 0x41     ; A = 'A'
0007  2A          STORE [H], A
0008  2B          INCH
0009  2D          HLOW              ; A = H & 0xFF
000A  18          INC A
000B  19          DEC A             ; Z = (молодший байт H == 0)
000C  32 05 00    JNZ 0x0005
000F  26          MOV A, B
0010  19          DEC A
0011  27          MOV B, A          ; B = B - 1
0012  32 05 00    JNZ 0x0005        ; ще одне коло
0015  04 00 08    OUTS 0x0800       ; 0x800-0x9FF: 512 байтів A без нуля
0018  00          HALT
```

Надруковано рівно 256 `A`, хоча `A` лежать аж до `0x9FF`:

```txt
ember> set 0 0x28
ember> set 1 0x00
ember> set 2 0x08
ember> set 3 0x21
ember> set 4 0x02
ember> set 5 0x20
ember> set 6 0x41
ember> set 7 0x2A
ember> set 8 0x2B
ember> set 9 0x2D
ember> set 10 0x18
ember> set 11 0x19
ember> set 12 0x32
ember> set 13 0x05
ember> set 14 0x00
ember> set 15 0x26
ember> set 16 0x19
ember> set 17 0x27
ember> set 18 0x32
ember> set 19 0x05
ember> set 20 0x00
ember> set 21 0x04
ember> set 22 0x00
ember> set 23 0x08
ember> set 24 0x00
ember> run
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
halted after 3596 steps
ember> dump 0x9F0 32
09f0  41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  |AAAAAAAAAAAAAAAA|
0a00  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> quit
```

Операнд за коробкою — помилка до будь-якого друку; потім `set16 1 0x0FFC` робить з тієї самої інструкції `OUTS 0x0FFC`: `W X Y Z` на `0xFFC`–`0xFFF` без нуля, і друк обривається на краю коробки:

```txt
адр.  байти       текст
0000  04 00 10    OUTS 0x1000       ; операнд за коробкою: помилка
0003  00          HALT
```

```txt
ember> set 0xFFC 0x57
ember> set 0xFFD 0x58
ember> set 0xFFE 0x59
ember> set 0xFFF 0x5A
ember> set 0 0x04
ember> set16 1 0x1000
ember> set 3 0x00
ember> run
address 0x1000 is outside 0..4095 (OUTS 0x1000 at 0x0000); nothing done
stopped after 0 steps
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> set16 1 0x0FFC
ember> run
WXYZ
halted after 2 steps
ember> quit
```

## M5 — пошук по ділянці

Даний `find` з Lab 04 шукає байт, **рівний** `needle`. Перший ненульовий байт картинки — це перший байт, **не рівний** `0`, тому поруч у `src/search.cpp` — той самий цикл з `!=`. Два питання, один цикл:

```cpp
std::uint16_t find_not(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) != needle)
            return i;
    }
    return hi;
}
```

Регіон даних (`HELLO\0` на `0x800`): перше `L` — на `0x0802`; перший `0` — на `0x0805`, тобто довжина рядка `0x805 - 0x800 = 5` (`strlen`). Відеопам'ять: на порожньому екрані ненульового байта немає; після тих самих трьох пікселів, що в M2 (тут — командою `plot`), картинка починається на `0x0a12` — піксель `(20, 2)`: `2 * 8 + 20 / 8 = 18 = 0x12`, біт `7 - 4 = 3`, `1 << 3 = 8`:

```txt
ember> set 0x800 72
ember> set 0x801 69
ember> set 0x802 76
ember> set 0x803 76
ember> set 0x804 79
ember> set 0x805 0
ember> find 0x800 0xA00 0x4C
found 0x4c at 0x0802
ember> find 0x800 0xA00 0
found 0x00 at 0x0805
ember> find 0xA00 0xB00 !0
no byte != 0x00 in [0x0a00, 0x0b00)
ember> plot 20 2
ember> plot 10 3
ember> plot 63 31
ember> find 0xA00 0xB00 !0
found 0x08 at 0x0a12 (first byte != 0x00)
ember> find 2560 2816 8
found 0x08 at 0x0a12
ember> quit
```

## Advanced

### `sort <lo> <hi>`

`sort <lo> <hi> [bubble|insert]` сортує байти `[lo, hi)` за зростанням (беззнакові 0..255) на місці, тільки через `mem_get`/`mem_set` (`src/sort.cpp`). Бульбашка: прохід — один зовнішній цикл; прохід без жодного обміну — останній. Вставки: прохід — вставка елемента `i = 1..n-1` вліво обмінами сусідів. Обмін — читання двох комірок і два записи, а не `swap(Byte&, Byte&)`: посилання на `Byte` усередині `Memory::data` — це доступ до коробки повз `mem_get`/`mem_set`:

```cpp
static void swap_cells(Memory& mem, std::size_t i, std::size_t j) {
    const Byte tmp = mem_get(mem, i);
    mem_set(mem, i, mem_get(mem, j));
    mem_set(mem, j, tmp);
}
```

Ті самі 8 байтів на `0x800` — бульбашкою, потім вставками; посередині обидва алгоритми на вже відсортованих байтах (0 обмінів):

```txt
ember> set 0x800 0x96
ember> set 0x801 0x1E
ember> set 0x802 0x78
ember> set 0x803 0x3C
ember> set 0x804 0xF0
ember> set 0x805 0x00
ember> set 0x806 0x5A
ember> set 0x807 0xD2
ember> dump 0x800 8
0800  96 1e 78 3c f0 00 5a d2 00 00 00 00 00 00 00 00  |..x<..Z.........|
ember> sort 0x800 0x808
sorted [0x0800, 0x0808) by bubble: 6 passes, 27 compares, 13 swaps
ember> dump 0x800 8
0800  00 1e 3c 5a 78 96 d2 f0 00 00 00 00 00 00 00 00  |..<Zx...........|
ember> sort 0x800 0x808
sorted [0x0800, 0x0808) by bubble: 1 passes, 7 compares, 0 swaps
ember> sort 0x800 0x808 insert
sorted [0x0800, 0x0808) by insert: 7 passes, 7 compares, 0 swaps
ember> set 0x800 0x96
ember> set 0x801 0x1E
ember> set 0x802 0x78
ember> set 0x803 0x3C
ember> set 0x804 0xF0
ember> set 0x805 0x00
ember> set 0x806 0x5A
ember> set 0x807 0xD2
ember> sort 0x800 0x808 insert
sorted [0x0800, 0x0808) by insert: 7 passes, 18 compares, 13 swaps
ember> dump 0x800 8
0800  00 1e 3c 5a 78 96 d2 f0 00 00 00 00 00 00 00 00  |..<Zx...........|
ember> quit
```

### Сортування стовпчиками

`sort … show`: перед першим проходом (`before`) і після кожного (`pass k: m swaps`) — `clear`, стовпчики, `show()`. Стовпчик `i` — байт `mem[lo + i]`: ширина `64 / n` (останній піксель — проміжок), висота `(v + 7) / 8` пікселів знизу. Не більше 64 байтів; діапазон, що перетинає відеопам'ять, відхиляється — стовпчики затерли б самі байти. Екран після `sort … show` зайнятий стовпчиками. Усі кадри одного прогону:

```txt
ember> set 0x800 0x3C
ember> set 0x801 0x5A
ember> set 0x802 0x1E
ember> set 0x803 0x96
ember> set 0x804 0x78
ember> set 0x805 0xF0
ember> set 0x806 0xB4
ember> set 0x807 0xD2
ember> dump 0x800 8
0800  3c 5a 1e 96 78 f0 b4 d2 00 00 00 00 00 00 00 00  |<Z..x...........|
ember> sort 0x800 0x808 show
before
................................................................
................................................................
........................................#######.................
........................................#######.................
........................................#######.................
........................................#######.........#######.
........................................#######.........#######.
........................................#######.........#######.
........................................#######.........#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................#######.........#######.#######.#######.
........................#######.........#######.#######.#######.
........................#######.........#######.#######.#######.
........................#######.........#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........#######.........#######.#######.#######.#######.#######.
........#######.........#######.#######.#######.#######.#######.
........#######.........#######.#######.#######.#######.#######.
........#######.........#######.#######.#######.#######.#######.
#######.#######.........#######.#######.#######.#######.#######.
#######.#######.........#######.#######.#######.#######.#######.
#######.#######.........#######.#######.#######.#######.#######.
#######.#######.........#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
pass 1: 4 swaps
................................................................
................................................................
........................................................#######.
........................................................#######.
........................................................#######.
................................................#######.#######.
................................................#######.#######.
................................................#######.#######.
................................................#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
#######.........#######.#######.#######.#######.#######.#######.
#######.........#######.#######.#######.#######.#######.#######.
#######.........#######.#######.#######.#######.#######.#######.
#######.........#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
pass 2: 1 swaps
................................................................
................................................................
........................................................#######.
........................................................#######.
........................................................#######.
................................................#######.#######.
................................................#######.#######.
................................................#######.#######.
................................................#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
pass 3: 0 swaps
................................................................
................................................................
........................................................#######.
........................................................#######.
........................................................#######.
................................................#######.#######.
................................................#######.#######.
................................................#######.#######.
................................................#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
........................................#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
................................#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
........................#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
................#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
........#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
#######.#######.#######.#######.#######.#######.#######.#######.
sorted [0x0800, 0x0808) by bubble: 3 passes, 18 compares, 5 swaps
ember> dump 0x800 8
0800  1e 3c 5a 78 96 b4 d2 f0 00 00 00 00 00 00 00 00  |.<Zx............|
ember> quit
```

### Піксель, що стрибає

`anim <steps> <frames>` — цикл на хості: `frames` разів `run()` щонайбільше `steps` кроків, потім `frame k: …` і `show()`. Це той самий `run()`, що в команди `run`, з новим параметром — лімітом кроків (без нього — `RUN_STEP_LIMIT`, як у Lab 04); точка `b` і `trace` діють. `HALT` або помилка зупиняють анімацію з тим самим повідомленням, що в `run`.

```cpp
RunResult run(CPU& cpu, bool trace, long breakpoint, unsigned long limit = RUN_STEP_LIMIT);
```

Гість: `x`, `y`, `dx`, `dy` на `0x800`–`0x803`, нові `x`, `y` — на `0x804`/`0x805`; від стінки `dx` (`dy`) міняє знак. `UNPLOT` старого місця і `PLOT` нового стоять поруч, тому піксель темний лише між ними:

```txt
адр.  байти       текст
0000  22 00 08    LOAD A, [0x0800]  ; A = x
0003  23 02 08    LOAD B, [0x0802]  ; B = dx (1 або 0xFF = -1)
0006  10          ADD A, B          ; A = x + dx; Z = (A == 0)
0007  24 04 08    STORE [0x0804], A ; нове x
000A  31 13 00    JZ 0x0013         ; ліва стінка
000D  21 3F       LOADI B, 63
000F  1A          CMP A, B          ; x == 63?
0010  32 1C 00    JNZ 0x001C        ; ні: стінки немає
0013  20 00       LOADI A, 0x00
0015  23 02 08    LOAD B, [0x0802]
0018  11          SUB A, B          ; A = 0 - dx
0019  24 02 08    STORE [0x0802], A ; dx = -dx
001C  22 01 08    LOAD A, [0x0801]  ; те саме для y
001F  23 03 08    LOAD B, [0x0803]
0022  10          ADD A, B
0023  24 05 08    STORE [0x0805], A ; нове y
0026  31 2F 00    JZ 0x002F         ; верхня стінка
0029  21 1F       LOADI B, 31
002B  1A          CMP A, B          ; y == 31?
002C  32 38 00    JNZ 0x0038
002F  20 00       LOADI A, 0x00
0031  23 03 08    LOAD B, [0x0803]
0034  11          SUB A, B
0035  24 03 08    STORE [0x0803], A ; dy = -dy
0038  22 00 08    LOAD A, [0x0800]
003B  23 01 08    LOAD B, [0x0801]
003E  42          UNPLOT            ; погасити старе місце
003F  22 04 08    LOAD A, [0x0804]
0042  24 00 08    STORE [0x0800], A ; x = нове x
0045  23 05 08    LOAD B, [0x0805]
0048  25 01 08    STORE [0x0801], B ; y = нове y
004B  41          PLOT              ; засвітити нове
004C  30 00 00    JMP 0x0000
```

З `b 0` кадр — рівно одна ітерація циклу гостя (25 кроків, з відскоком — 29): `x` 62 → 63 → 62. Без точки — кадр кожні 30 кроків, де б не був `PC`:

```txt
ember> set 0 0x22
ember> set 1 0x00
ember> set 2 0x08
ember> set 3 0x23
ember> set 4 0x02
ember> set 5 0x08
ember> set 6 0x10
ember> set 7 0x24
ember> set 8 0x04
ember> set 9 0x08
ember> set 10 0x31
ember> set 11 0x13
ember> set 12 0x00
ember> set 13 0x21
ember> set 14 0x3F
ember> set 15 0x1A
ember> set 16 0x32
ember> set 17 0x1C
ember> set 18 0x00
ember> set 19 0x20
ember> set 20 0x00
ember> set 21 0x23
ember> set 22 0x02
ember> set 23 0x08
ember> set 24 0x11
ember> set 25 0x24
ember> set 26 0x02
ember> set 27 0x08
ember> set 28 0x22
ember> set 29 0x01
ember> set 30 0x08
ember> set 31 0x23
ember> set 32 0x03
ember> set 33 0x08
ember> set 34 0x10
ember> set 35 0x24
ember> set 36 0x05
ember> set 37 0x08
ember> set 38 0x31
ember> set 39 0x2F
ember> set 40 0x00
ember> set 41 0x21
ember> set 42 0x1F
ember> set 43 0x1A
ember> set 44 0x32
ember> set 45 0x38
ember> set 46 0x00
ember> set 47 0x20
ember> set 48 0x00
ember> set 49 0x23
ember> set 50 0x03
ember> set 51 0x08
ember> set 52 0x11
ember> set 53 0x24
ember> set 54 0x03
ember> set 55 0x08
ember> set 56 0x22
ember> set 57 0x00
ember> set 58 0x08
ember> set 59 0x23
ember> set 60 0x01
ember> set 61 0x08
ember> set 62 0x42
ember> set 63 0x22
ember> set 64 0x04
ember> set 65 0x08
ember> set 66 0x24
ember> set 67 0x00
ember> set 68 0x08
ember> set 69 0x23
ember> set 70 0x05
ember> set 71 0x08
ember> set 72 0x25
ember> set 73 0x01
ember> set 74 0x08
ember> set 75 0x41
ember> set 76 0x30
ember> set 77 0x00
ember> set 78 0x00
ember> set 0x800 61
ember> set 0x801 2
ember> set 0x802 1
ember> set 0x803 1
ember> b 0
breakpoint at 0x0000
ember> anim 1000 3
frame 1: 25 steps, PC=0x0000
................................................................
................................................................
................................................................
..............................................................#.
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
frame 2: 29 steps, PC=0x0000
................................................................
................................................................
................................................................
................................................................
...............................................................#
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
frame 3: 25 steps, PC=0x0000
................................................................
................................................................
................................................................
................................................................
................................................................
..............................................................#.
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
ember> b off
no breakpoint
ember> anim 30 2
frame 1: 30 steps, PC=0x000d
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
.............................................................#..
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
frame 2: 30 steps, PC=0x0022
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
............................................................#...
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
ember> dump 0x800 6
0800  3c 07 ff 01 3b 07 00 00 00 00 00 00 00 00 00 00  |<...;...........|
ember> quit
```

### `rowmin <y>`

`rowmin` іде рядком зліва направо через `pixel()` — ту саму формулу, читання через `mem_get`. Ті самі три пікселі, що в M2 (командою `plot`), потім рамка:

```txt
ember> plot 20 2
ember> plot 10 3
ember> plot 63 31
ember> rowmin 2
row 2: leftmost lit pixel at x=20
ember> rowmin 3
row 3: leftmost lit pixel at x=10
ember> rowmin 31
row 31: leftmost lit pixel at x=63
ember> rowmin 0
row 0: no lit pixels
ember> rowmin 32
row is 0..31, got 32
ember> rowmin x
usage: rowmin <y>
ember> cls
ember> frame
ember> rowmin 5
row 5: leftmost lit pixel at x=0
ember> quit
```

## Якщо встигаєте

Сортування з дампами до й після, стовпчики після кожного проходу, анімація з `show` кожні N кроків і `rowmin` — у розділі Advanced вище.

### Порт виводу й таймер

`0xB00` — `PORT_OUT`, `0xB01` — `TICKS` (рядки ISA §2 — у таблиці «Реалізовані рядки» нижче). Обидва живуть в одному місці — `guest_read`/`guest_write` у `src/cpu.cpp`, через які гість звертається до пам'яті за адресою (`LOAD`/`STORE` за `addr16` і за `H`, `OUTS`). Хост (`set`, `get`, `plot`, `sort`) ходить через `mem_get`/`mem_set` і бачить там звичайні байти:

```cpp
static bool guest_read(const CPU& cpu, std::size_t addr, Byte& out) {
    if (addr >= MEM_SIZE)
        return false;
    if (addr == TICKS)
        // ticks does not count this instruction yet: it is counted when it ends.
        out = static_cast<Byte>(cpu.ticks + 1);
    else
        out = mem_get(*cpu.mem, addr);
    return true;
}

static bool guest_write(CPU& cpu, std::size_t addr, Byte value) {
    if (!mem_set(*cpu.mem, addr, value))
        return false; // nothing written: addr >= MEM_SIZE
    if (addr == PORT_OUT)
        guest_putc(value); // every write prints, even the same byte twice
    return true;
}
```

`ticks` — нове поле `CPU` (`Byte`, тобто саме за модулем 256): `++` після кожного успішного кроку (`step` і `run`, разом із `HALT`). Поки інструкція виконується, вона ще не порахована, тому читання дає `ticks + 1`.

Запис гостя друкує, а байт лишається в пам'яті: `get 2816` → `73` (`'I'`). `set 2816 65` на хості нічого не друкує:

```txt
адр.  байти       текст
0000  20 48       LOADI A, 0x48     ; 'H'
0002  24 00 0B    STORE [0x0B00], A
0005  20 49       LOADI A, 0x49     ; 'I'
0007  24 00 0B    STORE [0x0B00], A
000A  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x48
ember> set 2 0x24
ember> set 3 0x00
ember> set 4 0x0B
ember> set 5 0x20
ember> set 6 0x49
ember> set 7 0x24
ember> set 8 0x00
ember> set 9 0x0B
ember> set 10 0x00
ember> run
HI
halted after 5 steps
ember> get 2816
73  0x49  0b01001001  'I'
ember> set 2816 65
ember> get 2816
65  0x41  0b01000001  'A'
ember> quit
```

Те саме через `H`:

```txt
адр.  байти       текст
0000  28 00 0B    LOADH H, 0x0B00
0003  20 4F       LOADI A, 0x4F     ; 'O'
0005  2A          STORE [H], A
0006  20 4B       LOADI A, 0x4B     ; 'K'
0008  2A          STORE [H], A
0009  00          HALT
```

```txt
ember> set 0 0x28
ember> set 1 0x00
ember> set 2 0x0B
ember> set 3 0x20
ember> set 4 0x4F
ember> set 5 0x2A
ember> set 6 0x20
ember> set 7 0x4B
ember> set 8 0x2A
ember> set 9 0x00
ember> run
OK
halted after 6 steps
ember> get 2816
75  0x4b  0b01001011  'K'
ember> quit
```

Той самий байт двічі — два символи:

```txt
адр.  байти       текст
0000  20 21       LOADI A, 0x21     ; '!'
0002  24 00 0B    STORE [0x0B00], A
0005  24 00 0B    STORE [0x0B00], A ; той самий байт удруге
0008  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x21
ember> set 2 0x24
ember> set 3 0x00
ember> set 4 0x0B
ember> set 5 0x24
ember> set 6 0x00
ember> set 7 0x0B
ember> set 8 0x00
ember> run
!!
halted after 4 steps
ember> get 2816
33  0x21  0b00100001  '!'
ember> quit
```

В `ember` кожен `STORE` — окремий виклик `guest_write`, і друк відбувається на кожному виклику, тому два однакові записи — два `!`. Компілятор C++ бачить `*port = '!'; *port = '!';` як звичайну пам'ять: між записами ніхто не читає, перший запис «мертвий» (dead store), і оптимізатор має право залишити один. Справжній порт (регістр `ODR` STM32 з HARDWARE.md, Lab 3) тоді отримав би один символ замість двох. `volatile` робить кожен доступ спостережуваною дією, яку не можна ні викинути, ні злити з іншою, ні переставити. Compiler Explorer, `g162` (x86-64 gcc 16.2), `-O2`, ті самі фільтри, що в Lab 04:

```cpp
void twice(unsigned char* port) {
    *port = '!';
    *port = '!';
}

void twice_volatile(volatile unsigned char* port) {
    *port = '!';
    *port = '!';
}
```

`body.json`:

```json
{
  "source": "void twice(unsigned char* port) {\n    *port = '!';\n    *port = '!';\n}\n\nvoid twice_volatile(volatile unsigned char* port) {\n    *port = '!';\n    *port = '!';\n}\n",
  "options": {
    "userArguments": "-O2",
    "filters": {
      "intel": true,
      "directives": true,
      "labels": true,
      "commentOnly": true,
      "demangle": true
    }
  }
}
```

```bash
curl -s -X POST -H 'Content-Type: application/json' -H 'Accept: text/plain' \
     --data @body.json https://godbolt.org/api/compiler/g162/compile
```

```txt
# Compilation provided by Compiler Explorer at https://godbolt.org/
"twice(unsigned char*)":
        mov     BYTE PTR [rdi], 33
        ret
"twice_volatile(unsigned char volatile*)":
        mov     BYTE PTR [rdi], 33
        mov     BYTE PTR [rdi], 33
        ret
```

Без `volatile` — одна `mov BYTE PTR [rdi], 33`, з `volatile` — дві.

Таймер: `LOAD A, [0x0B01]` — третя інструкція, тому `A = 3`. За `0xB01` у пам'яті лічильника немає: `get 2817` на хості — звичайний байт (`0`):

```txt
адр.  байти       текст
0000  01          NOP
0001  01          NOP
0002  22 01 0B    LOAD A, [0x0B01]  ; третя інструкція: A = 3
0005  03          OUTN
0006  00          HALT
```

```txt
ember> set 0 0x01
ember> set 1 0x01
ember> set 2 0x22
ember> set 3 0x01
ember> set 4 0x0B
ember> set 5 0x03
ember> set 6 0x00
ember> run
3 
halted after 5 steps
ember> get 2817
0  0x00  0b00000000  '.'
ember> quit
```

### Байт стану переїхав на `0xB02`

Байт стану (власне розширення Lab 02, дзеркало прапорців `0b00000ZNC`) лежав на `0xB00`. ISA §2 віддає `0xB00`/`0xB01` під `PORT_OUT`/`TICKS`, тож з цієї лаби байт стану — на `0xB02` (`STATUS_ADDR` у `src/cpu.hpp`); інакше кожен крок писав би прапорці в порт. Він пишеться через `mem_set`, тому нічого не друкує. Після `CMP` з `Z = 1`:

```txt
адр.  байти       текст
0000  1A          CMP A, B          ; A = B = 5: Z = 1
0001  00          HALT
```

```txt
ember> reg a 5
ember> reg b 5
ember> set 0 0x1A
ember> set 1 0x00
ember> run
halted after 2 steps
ember> regs
PC=0x0002  A=5  B=5  H=0x0000  Z=1 N=0 C=0
ember> get 2818
4  0x04  0b00000100  '.'
ember> get 2816
0  0x00  0b00000000  '.'
ember> quit
```

### `UNPLOT`

`UNPLOT` (`0x42`, ISA *opt*) — той самий біт, скинутий: `mem_get(...) & ~mask`; `C` — як у `PLOT`. `(3, 3)` і `(4, 3)` — біти 4 і 3 байта `0xA18`; `UNPLOT` гасить `(3, 3)`, сусід лишається; `UNPLOT` за екраном — `C = 1`, нічого не змінено:

```txt
адр.  байти       текст
0000  20 03       LOADI A, 3
0002  21 03       LOADI B, 3
0004  42          UNPLOT            ; (3, 3): C = 0
0005  20 40       LOADI A, 64
0007  42          UNPLOT            ; (64, 3): за екраном -> C = 1
0008  00          HALT
```

```txt
ember> plot 3 3
ember> plot 4 3
ember> get 0xA18
24  0x18  0b00011000  '.'
ember> set 0 0x20
ember> set 1 0x03
ember> set 2 0x21
ember> set 3 0x03
ember> set 4 0x42
ember> set 5 0x20
ember> set 6 0x40
ember> set 7 0x42
ember> set 8 0x00
ember> trace on
ember> run
0000  20 03     LOADI A, 0x03     -> A=  3  B=  0  H=0x0000  Z=0 N=0 C=0
0002  21 03     LOADI B, 0x03     -> A=  3  B=  3  H=0x0000  Z=0 N=0 C=0
0004  42        UNPLOT            -> A=  3  B=  3  H=0x0000  Z=0 N=0 C=0
0005  20 40     LOADI A, 0x40     -> A= 64  B=  3  H=0x0000  Z=0 N=0 C=0
0007  42        UNPLOT            -> A= 64  B=  3  H=0x0000  Z=0 N=0 C=1
0008  00        HALT              -> A= 64  B=  3  H=0x0000  Z=0 N=0 C=1
halted after 6 steps
ember> get 0xA18
8  0x08  0b00001000  '.'
ember> unplot 4 3
ember> get 0xA18
0  0x00  0b00000000  '.'
ember> unplot 64 0
pixel (64, 0) is outside the 64x32 screen; nothing changed
ember> quit
```

### Сортування байткодом

Бульбашка для 8 байтів на `0x800` у байткоді `ember`: `H` ходить по парах `mem[i]`, `mem[i+1]`, `CMP A, B` + `JNC` — чи вже по порядку (беззнаково), прапорець «був обмін» — на `0x808`:

```txt
адр.  байти       текст
0000  20 00       LOADI A, 0x00
0002  24 08 08    STORE [0x0808], A ; swapped = 0
0005  28 00 08    LOADH H, 0x0800   ; H = &mem[0]
0008  29          LOAD A, [H]
0009  27          MOV B, A          ; B = mem[i]
000A  2B          INCH
000B  29          LOAD A, [H]       ; A = mem[i+1]
000C  1A          CMP A, B          ; C = 1, якщо mem[i+1] < mem[i]
000D  34 1A 00    JNC 0x001A        ; уже по порядку
0010  2C          DECH
0011  2A          STORE [H], A      ; mem[i] = менший
0012  2B          INCH
0013  26          MOV A, B
0014  2A          STORE [H], A      ; mem[i+1] = більший
0015  20 01       LOADI A, 0x01
0017  24 08 08    STORE [0x0808], A ; swapped = 1
001A  2D          HLOW              ; A = H & 0xFF = i + 1
001B  21 07       LOADI B, 0x07
001D  1A          CMP A, B
001E  32 08 00    JNZ 0x0008        ; ще не дійшли до пари (6, 7)
0021  22 08 08    LOAD A, [0x0808]
0024  21 00       LOADI B, 0x00
0026  1A          CMP A, B
0027  32 00 00    JNZ 0x0000        ; був обмін: ще прохід
002A  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x00
ember> set 2 0x24
ember> set 3 0x08
ember> set 4 0x08
ember> set 5 0x28
ember> set 6 0x00
ember> set 7 0x08
ember> set 8 0x29
ember> set 9 0x27
ember> set 10 0x2B
ember> set 11 0x29
ember> set 12 0x1A
ember> set 13 0x34
ember> set 14 0x1A
ember> set 15 0x00
ember> set 16 0x2C
ember> set 17 0x2A
ember> set 18 0x2B
ember> set 19 0x26
ember> set 20 0x2A
ember> set 21 0x20
ember> set 22 0x01
ember> set 23 0x24
ember> set 24 0x08
ember> set 25 0x08
ember> set 26 0x2D
ember> set 27 0x21
ember> set 28 0x07
ember> set 29 0x1A
ember> set 30 0x32
ember> set 31 0x08
ember> set 32 0x00
ember> set 33 0x22
ember> set 34 0x08
ember> set 35 0x08
ember> set 36 0x21
ember> set 37 0x00
ember> set 38 0x1A
ember> set 39 0x32
ember> set 40 0x00
ember> set 41 0x00
ember> set 42 0x00
ember> set 0x800 0x96
ember> set 0x801 0x1E
ember> set 0x802 0x78
ember> set 0x803 0x3C
ember> set 0x804 0xF0
ember> set 0x805 0x00
ember> set 0x806 0x5A
ember> set 0x807 0xD2
ember> dump 0x800 8
0800  96 1e 78 3c f0 00 5a d2 00 00 00 00 00 00 00 00  |..x<..Z.........|
ember> run
halted after 554 steps
ember> dump 0x800 8
0800  00 1e 3c 5a 78 96 d2 f0 00 00 00 00 00 00 00 00  |..<Zx...........|
ember> quit
```

## Реалізовані рядки [ISA.uk.md](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md) (Лаба 5)

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x04` | `OUTS addr16` | 3 | — | друкувати байти з `addr`, поки не трапиться `0` або поки не набереться 256 байтів | 5 |
| `0x40` | `CLS` | 1 | — | занулити всі 256 байтів відеопам'яті | 5 |
| `0x41` | `PLOT` | 1 | C | засвітити піксель `x = A`, `y = B`. За межами екрана: виставити `C` і нічого не міняти | 5 |
| `0x42` | `UNPLOT` | 1 | C | *opt* погасити той самий піксель | 5 |
| `0x43` | `SHOW` | 1 | — | намалювати відеопам'ять у терміналі | 5 |

| Регіон | Діапазон | Константа | З'являється в |
|---|---|---|---|
| Відеопам'ять | `0xA00`–`0xAFF` | `VRAM_LO`, `VRAM_HI` | Lab 5 |

| Адреса | Назва | Що робить |
|---|---|---|
| `0xB00` | `PORT_OUT` | запис гостя (`STORE [0x0B00], A`, `STORE [H], A` тощо) **одразу** друкує байт як символ. Сам байт теж лишається в пам'яті |
| `0xB01` | `TICKS` | читання гостя (`LOAD A, [0x0B01]`, `LOAD A, [H]`) повертає, скільки інструкцій виконано разом із цією, за модулем 256 |

## Власні розширення

| Регіон | Діапазон | Константа | З'являється в |
|---|---|---|---|
| Байт стану | `0xB02` | `STATUS_ADDR` | Лаба 2 (з Лаби 5 — `0xB02`) |

Нових власних опкодів у Лабі 5 немає; `JG`/`JL` (`0x70`/`0x71`) — як у Lab 04. Уточнення рядків ISA: `PLOT`/`UNPLOT` на екрані виставляють `C = 0`; `OUTS` з операндом `>= 4096` — помилка, а рядок, що доходить до `0xFFF`, обривається на краю коробки.

---

# Lab 06 — Іменовані набори: структури, переліки й купа

Нові й змінені команди REPL:

| Команда | Що робить |
|---|---|
| `layout [cpu\|flags\|instruction\|sprite\|value]` | зміщення й розмір кожного поля (`offsetof`/`sizeof`), рядки `(padding)` — дірки між полями й у кінці, підсумок `fields N + padding M = sizeof`; без аргументу — усі п'ять структур |
| `hostdump <cpu\|sprites>` | сирі байти хостової структури `CPU` (як у Lab 03) або масиву `sprites` |
| `sprite <i> <x> <y> <vx> <vy>` | заповнити спрайт `i` (0..7) і зробити живим: `x` 0..63, `y` 0..31, `vx` −63..63, `vy` −31..31; перемалювати відеопам'ять |
| `sprite <i> off` | спрайт `i` більше не живий; перемалювати |
| `sprites` | усі 8 записів |
| `tick [n]` | `n` разів (1..100, за замовчуванням 1): зсунути всі живі спрайти з відскоком від країв 64×32, перемалювати, `show` |
| `heap` | стан купи гостя: `heap_ptr`, скільки зайнято й лишилось, список вільних блоків |
| `alloc <n>` / `free <addr> <n>` | ті самі `heap_alloc`/`heap_free`, що за `ALLOC`/`FREE`, але від імені хоста: регістри й прапорці не чіпають |
| `value <byte\|addr\|sprite> <n>` | зробити тегований `Value` і надрукувати його за тегом |

Рядок трасування той самий, що в Lab 03–05; нові мнемоніки — `ALLOC` і `FREE`. Нова помилка кроку — `free list is broken at 0x…; nothing done` (див. «Якщо встигаєте»).

## M1 — `enum class Op` і розкладка структур

Вихідної точки лаби — констант `const Byte OP_*` у `cpu.hpp` — у моєму коді не було ніколи: `enum class Op : std::uint8_t` зі значеннями з ISA живе в `src/opcodes.hpp` від Lab 02, і `step` уже тоді перемикався по `Op`. Тож рефакторинг у цій лабі — лише нові рядки (`Alloc`, `Free`) і `struct Instruction` нижче:

```cpp
enum class Op : std::uint8_t {
    // 0x0_ — control and output
    Halt = 0x00,
    Nop = 0x01,

    // 0x6_ — system (Lab 6)
    Alloc = 0x60, // H = a new block of A bytes on the guest heap; C = 1: it does not fit
    // 0x7_ — own extensions (ISA §8: 0x70-0xFF are free), Lab 4 and Lab 6
    Jg = 0x70,   // jump if A > B as signed bytes (-128..127); flags untouched
    Jl = 0x71,   // jump if A < B as signed bytes (-128..127); flags untouched
    Free = 0x72, // own (Lab 6): give [H, H + A) back to the heap; C = 1: refused
```

```txt
d9285a0 Lab 02: ALU with Z/N/C flags, CPU step/run/regs, decode by group nibble
```

`struct CPU` і `struct Flags` — структури з Lab 02. Крок проєкту лаби — «`CPU` і `Instruction` як структури»: `Instruction` — те, що `step` прочитав за `PC` до виконання. Сирий байт стає `Op` рівно в одному місці, у `fetch()`, одразу після того, як `instr_size()` визнав його рядком ISA; далі `switch` (група, потім `Op`), мнемоніка й трасування читають `ins`:

```cpp
// What step read at PC before executing it: the ISA row and the bytes.
// `op` is an Op only once instr_size knew the byte.
struct Instruction {
    std::uint16_t at;  // its address: PC before the step
    Op op;             // bytes[0] as an Op
    Byte size;         // 1..3, the "розмір" column of the ISA
    Byte bytes[3];     // opcode, then operands; the unused ones are 0
    std::uint16_t w16; // bytes[1..2] as one value, low byte first (size 3 only)
};
```

```cpp
static StepResult fetch(const Memory& mem, std::size_t at, Instruction& ins) {
    if (at >= MEM_SIZE)
        return StepResult::out_of_memory;
    const Byte op = mem_get(mem, at);
    const std::uint16_t size = instr_size(op);
    if (size == 0)
        return StepResult::bad_opcode;
    // The one bounds check on the instruction stream: never read an operand
    // past the last byte of the box.
    if (at + size > MEM_SIZE)
        return StepResult::out_of_memory;

    ins.at = static_cast<std::uint16_t>(at);
    ins.op = static_cast<Op>(op);
    ins.size = static_cast<Byte>(size);
    ins.bytes[0] = op;
    ins.bytes[1] = size > 1 ? mem_get(mem, at + 1) : Byte{0};
    ins.bytes[2] = size > 2 ? mem_get(mem, at + 2) : Byte{0};
    ins.w16 = size == 3 ? get16(mem, at + 1) : 0;
    return StepResult::ok;
}

StepResult step(CPU& cpu, bool trace) {
    if (cpu.halted)
        return StepResult::halted;

    Instruction ins{};
    const StepResult fetched = fetch(*cpu.mem, cpu.pc, ins);
    if (fetched != StepResult::ok)
        return fetched;
```

Невідомий байт і далі помилка, а не тихий `Nop`: `0x61` — у групі `0x6_`, але рядка немає; `0x73` — у вільному діапазоні, але не наш опкод:

```txt
ember> set 0 0x61
ember> step
unknown opcode 0x61 at 0x0000; nothing done
ember> set 0 0x73
ember> step
unknown opcode 0x73 at 0x0000; nothing done
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> quit
```

**Поведінка не змінилась.** Файлу `checks/lab-06.txt` немає: перевірка Лаби 6 — усі перевірки Lab 1–5 без змін. Скрипт збирає Lab 05 (коміт `c4eca13`) поруч і проганяє обидві збірки на всіх п'яти файлах перевірок:

```sh
#!/bin/sh
# Lab 05 (c4eca13) against the working tree: every check file, both binaries.
old=$(mktemp -d)
git archive c4eca13 src CMakeLists.txt | tar -x -C "$old"
cmake -S "$old" -B "$old/build" -DCMAKE_BUILD_TYPE=Debug > /dev/null
cmake --build "$old/build" > /dev/null
for f in docs/checks/lab-01.txt notes-local/lab-0[2-5]/upstream/checks/lab-0?.txt; do
    "$old/build/ember" < "$f" > "$old/a.out" 2> "$old/a.err"; ea=$?
    ./build/ember < "$f" > "$old/b.out" 2> "$old/b.err"; eb=$?
    if cmp -s "$old/a.out" "$old/b.out" && [ ! -s "$old/a.err" ] && [ ! -s "$old/b.err" ] &&
       [ $ea -eq 0 ] && [ $eb -eq 0 ]; then
        echo "$(basename "$f"): identical, $(wc -l < "$old/b.out") lines, exit 0, stderr empty"
    else
        echo "$(basename "$f"): DIFFERENT (exit $ea/$eb)"
    fi
done
rm -rf "$old"
```

```txt
lab-01.txt: identical, 262 lines, exit 0, stderr empty
lab-02.txt: identical, 7 lines, exit 0, stderr empty
lab-03.txt: identical, 4 lines, exit 0, stderr empty
lab-04.txt: identical, 4 lines, exit 0, stderr empty
lab-05.txt: identical, 35 lines, exit 0, stderr empty
```

Дві з них — повністю, на поточній збірці. `checks/lab-02.txt`:

```txt
ember> reg a 200
ember> reg b 100
ember> set 0 0x10
ember> set 1 0x00
ember> regs
PC=0x0000  A=200  B=100  H=0x0000  Z=0 N=0 C=0
ember> step
0000  10        ADD A, B          -> A= 44  B=100  H=0x0000  Z=0 N=0 C=1
ember> regs
PC=0x0001  A=44  B=100  H=0x0000  Z=0 N=0 C=1
ember> step
0001  00        HALT              -> A= 44  B=100  H=0x0000  Z=0 N=0 C=1
ember> step
halted: step does nothing after HALT
ember> regs
PC=0x0002  A=44  B=100  H=0x0000  Z=0 N=0 C=1
ember> quit
```

`checks/lab-04.txt` (відлік `3 2 1`):

```txt
ember> set 0 0x20
ember> set 1 0x03
ember> set 2 0x03
ember> set 3 0x19
ember> set 4 0x32
ember> set 5 0x02
ember> set 6 0x00
ember> set 7 0x00
ember> run
3 2 1 
halted after 11 steps
ember> regs
PC=0x0008  A=0  B=0  H=0x0000  Z=1 N=0 C=0
ember> quit
```

### `sizeof(CPU)` і вирівнювальні байти

```cpp
struct CPU {
    Memory* mem;
    std::uint16_t pc = 0; // address of the NEXT instruction
    std::uint16_t h = 0;  // address register: where LOAD A, [H] reads
    Byte a = 0, b = 0;
    Flags f{};
    bool halted = false; // set by HALT; every later step refuses
    Byte ticks = 0;      // instructions executed, mod 256 (the guest reads it at TICKS)
    // The guest heap (Lab 6), not registers: `regs` does not show them.
    // heap_ptr needs an even address, so one padding byte sits before it.
    std::uint16_t heap_ptr = HEAP_LO; // the next fresh block starts here (ALLOC)
    std::uint16_t free_head = 0;      // first block of the free list (FREE); 0 = none
};
```

```txt
ember> layout cpu
CPU: sizeof=24 alignof=8
  off size field
    0    8 mem
    8    2 pc
   10    2 h
   12    1 a
   13    1 b
   14    3 f (Flags)
   17    1 halted
   18    1 ticks
   19    1 (padding)
   20    2 heap_ptr
   22    2 free_head
  fields 23 + padding 1 = 24
ember> layout flags
Flags: sizeof=3 alignof=1
  off size field
    0    1 z
    1    1 n
    2    1 c
  fields 3 + padding 0 = 3
ember> layout instruction
Instruction: sizeof=10 alignof=2
  off size field
    0    2 at
    2    1 op (Op)
    3    1 size
    4    3 bytes[3]
    7    1 (padding)
    8    2 w16
  fields 9 + padding 1 = 10
ember> reg a 0x41
ember> reg b 0x42
ember> reg h 0x0800
ember> reg pc 0x0123
ember> hostdump cpu
&cpu=0x7b93b86f10a0  sizeof(cpu)=24  sizeof(cpu.mem)=8  sizeof(*cpu.mem)=4096
0000  20 00 6f b8 93 7b 00 00 23 01 00 08 41 42 00 00  | .o..{..#...AB..|
0010  00 00 00 00 00 0c 00 00                          |........|
ember> quit
```

**`sizeof(CPU)` = 24**: полів 23 байти, один вирівнювальний байт на зміщенні 19. `heap_ptr` — `std::uint16_t`, йому потрібна парна адреса, а після `ticks` (зміщення 18) вільне непарне 19, тож компілятор пропускає байт. Хвоста немає: `CPU` містить `Memory*` (вирівнювання 8), тому `sizeof(CPU)` кратний 8, а 24 уже кратне. `Flags` — три `bool`, 3 байти без дірок. В `Instruction` дірка на 7: три байти `bytes` закінчуються на непарній адресі, а `w16` має стати на парну. У `hostdump cpu` це четвертий байт другого рядка дампу (зміщення `0x13` = 19), одразу перед `00 0c` (`heap_ptr = 0x0C00`, молодший байт уперед). Зараз там `00`, але значення таких байтів стандарт не гарантує: де дірки — кажуть `offsetof`/`sizeof` (`layout`), дамп лише показує, де вони лежать.

## M2 — спрайти

`struct Sprite` — даний код теорії §1, у `src/sprite.hpp`; масив — `Sprite sprites[SPRITE_COUNT]{};` у `main` поруч із `CPU cpu` (`SPRITE_COUNT = 8`, `{}` занулює всі записи, тож усі мертві):

```cpp
struct Sprite {
    std::uint8_t x, y;
    std::int8_t vx, vy;
    bool alive;
};
```

Рух однієї осі: `pos + v` рахується в `int` (`63 + 63` не влазить в `int8_t`); вийшли за край — віддзеркалити позицію й обернути швидкість; опинились рівно на краю, летячи назовні, — обернути одразу (правило notes §5: на краю швидкість змінює знак). Швидкості обмежені 63/31, тож одного відбиття завжди досить:

```cpp
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
```

`tick`: зсунути всіх живих, `clear` + `plot` кожного живого (екран — картинка масиву записів; усе інше з екрана стирається), даний `show()`. Два спрайти біля протилежних кутів, обидва летять у кут і на першому ж тіку відскакують — `(1, 30)` → `(0, 31)` → `(1, 30)`, `(62, 1)` → `(63, 0)` → `(62, 1)`. `Sprite` — 5 байтів без дірок, масив — 8 записів по 5 байтів підряд (`vx = -1` у дампі — `ff`):

```txt
ember> layout sprite
Sprite: sizeof=5 alignof=1
  off size field
    0    1 x
    1    1 y
    2    1 vx
    3    1 vy
    4    1 alive
  fields 5 + padding 0 = 5
ember> sprite 0 1 30 -1 1
sprite 0: x=1 y=30 vx=-1 vy=1 alive
ember> sprite 1 62 1 1 -1
sprite 1: x=62 y=1 vx=1 vy=-1 alive
ember> sprites
sprite 0: x=1 y=30 vx=-1 vy=1 alive
sprite 1: x=62 y=1 vx=1 vy=-1 alive
sprite 2: x=0 y=0 vx=0 vy=0 dead
sprite 3: x=0 y=0 vx=0 vy=0 dead
sprite 4: x=0 y=0 vx=0 vy=0 dead
sprite 5: x=0 y=0 vx=0 vy=0 dead
sprite 6: x=0 y=0 vx=0 vy=0 dead
sprite 7: x=0 y=0 vx=0 vy=0 dead
ember> hostdump sprites
&sprites=0x7b2a3f6f10e0  sizeof(sprites)=40  sizeof(Sprite)=5
0000  01 1e ff 01 01 3e 01 01 ff 01 00 00 00 00 00 00  |.....>..........|
0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0020  00 00 00 00 00 00 00 00                          |........|
ember> tick 3
tick 1: 2 alive
...............................................................#
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
#...............................................................
tick 2: 2 alive
................................................................
..............................................................#.
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
.#..............................................................
................................................................
tick 3: 2 alive
................................................................
................................................................
.............................................................#..
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
................................................................
..#.............................................................
................................................................
................................................................
ember> quit
```

Помилки нічого не змінюють; `sprite <i> off` лишає запис, але гасить його:

```txt
ember> sprite 8 0 0 1 1
sprite index is 0..7, got 8
ember> sprite 0 64 0 1 1
x is 0..63, got 64
ember> sprite 0 0 32 1 1
y is 0..31, got 32
ember> sprite 0 0 0 64 0
vx is -63..63, got 64
ember> sprite 0 0 0 0 -32
vy is -31..31, got -32
ember> sprite 0 1 2
usage: sprite <i> <x> <y> <vx> <vy> | sprite <i> off
ember> tick 101
usage: tick [1..100]
ember> sprite 0 10 5 3 -2
sprite 0: x=10 y=5 vx=3 vy=-2 alive
ember> sprite 0 off
sprite 0: x=10 y=5 vx=3 vy=-2 dead
ember> sprites
sprite 0: x=10 y=5 vx=3 vy=-2 dead
sprite 1: x=0 y=0 vx=0 vy=0 dead
sprite 2: x=0 y=0 vx=0 vy=0 dead
sprite 3: x=0 y=0 vx=0 vy=0 dead
sprite 4: x=0 y=0 vx=0 vy=0 dead
sprite 5: x=0 y=0 vx=0 vy=0 dead
sprite 6: x=0 y=0 vx=0 vy=0 dead
sprite 7: x=0 y=0 vx=0 vy=0 dead
ember> quit
```

## M3 — `new`/`delete` на хості, свідомо

**Шлях — Linux** (ця машина): тут LeakSanitizer є, тож для витоку вставляю справжній звіт, а не пояснення. Тимчасовий `scratch.cpp` жив у тимчасовому каталозі поза репозиторієм; у git його немає, нижче — кожен фрагмент повністю. Компілятор — GCC 16.2.1 (`c++`), рядок із Notes 01 плюс `-g`, щоб у звіті був `scratch.cpp:рядок` (clang 22 дає звіти того самого вигляду). Запуск через `script` (псевдотермінал), щоб рядок `42`, надрукований до звіту, не загубився в буфері; `ASAN_OPTIONS=color=never` — без кольорових кодів; для витоку — `ASAN_OPTIONS=color=never:detect_leaks=1`, як у notes §4 (на Linux і так увімкнено):

```sh
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -g scratch.cpp -o scratch
ASAN_OPTIONS=color=never script -q -e -c ./scratch /dev/null
```

У звітах обрізано тільки кадри стека з `libc` і `_start` (позначено `…`) і все після рядка `SUMMARY` (карту тіньових байтів); решта — як надрукувалось.

### Витік (`new` і забути)

```cpp
int main() {
    new int{1};   // забули вказівник
}
```

```txt

=================================================================
==925541==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 4 byte(s) in 1 object(s) allocated from:
    #0 0x7f5c5a32d2a1 in operator new(unsigned long) (/usr/lib/libasan.so.8+0x12d2a1) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x56296df8f1bb in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/leak/scratch.cpp:2
    … (кадри libc і _start обрізано)

SUMMARY: AddressSanitizer: 4 byte(s) leaked in 1 allocation(s).
```

Код виходу 1. Висновок: об'єкт на 4 байти в купі живий до кінця програми, а єдиний вказівник на нього (результат `new`) ніде не збережено — звільнити його вже нікому.

Виправлення — зберегти вказівник і звільнити (або віддати його `std::unique_ptr`, див. «Якщо встигаєте»):

```cpp
int main() {
    int* p = new int{1};   // keep the pointer
    delete p;
}
```

Вивід порожній, код виходу 0. Те саме з `std::make_unique<int>(1)` замість `new`/`delete` — теж порожньо, код 0.

### Читання після звільнення

```cpp
#include <iostream>
int main() {
    int* p = new int{42};
    std::cout << *p << '\n';
    delete p;
    std::cout << *p << '\n';   // dangling
}
```

```txt
42
=================================================================
==925533==ERROR: AddressSanitizer: heap-use-after-free on address 0x7b3f6f7e0010 at pc 0x55e835e16534 bp 0x7ffe9bd6dbb0 sp 0x7ffe9bd6dba0
READ of size 4 at 0x7b3f6f7e0010 thread T0
    #0 0x55e835e16533 in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/uaf/scratch.cpp:6
    … (кадри libc і _start обрізано)

0x7b3f6f7e0010 is located 0 bytes inside of 4-byte region [0x7b3f6f7e0010,0x7b3f6f7e0014)
freed by thread T0 here:
    #0 0x7f1f7172e421 in operator delete(void*, unsigned long) (/usr/lib/libasan.so.8+0x12e421) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x55e835e163fb in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/uaf/scratch.cpp:5
    … (кадри libc і _start обрізано)

previously allocated by thread T0 here:
    #0 0x7f1f7172d2a1 in operator new(unsigned long) (/usr/lib/libasan.so.8+0x12d2a1) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x55e835e1620b in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/uaf/scratch.cpp:3
    … (кадри libc і _start обрізано)

SUMMARY: AddressSanitizer: heap-use-after-free /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/uaf/scratch.cpp:6 in main
```

Код виходу 1. Висновок: перше читання (`42`) законне, друге (рядок 6) читає 4 байти, які `delete` у рядку 5 уже повернув купі, — ASan показує, де пам'ять звільнили й де виділили.

Виправлення — читати, поки об'єкт живий, а після `delete` занулити вказівник:

```cpp
#include <iostream>
int main() {
    int* p = new int{42};
    std::cout << *p << '\n';
    std::cout << *p << '\n';   // read while the object is alive
    delete p;
    p = nullptr;               // no dangling pointer left
}
```

```txt
42
42
```

Звіту немає, код виходу 0.

### Подвійне звільнення

```cpp
#include <iostream>
int main() {
    int* p = new int{42};
    std::cout << *p << '\n';
    delete p;
    delete p;
}
```

```txt
42
=================================================================
==925537==ERROR: AddressSanitizer: attempting double-free on 0x7b76697e0010 in thread T0:
    #0 0x7f566b72e421 in operator delete(void*, unsigned long) (/usr/lib/libasan.so.8+0x12e421) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x55be3b506411 in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/df/scratch.cpp:6
    … (кадри libc і _start обрізано)

0x7b76697e0010 is located 0 bytes inside of 4-byte region [0x7b76697e0010,0x7b76697e0014)
freed by thread T0 here:
    #0 0x7f566b72e421 in operator delete(void*, unsigned long) (/usr/lib/libasan.so.8+0x12e421) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x55be3b5063fb in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/df/scratch.cpp:5
    … (кадри libc і _start обрізано)

previously allocated by thread T0 here:
    #0 0x7f566b72d2a1 in operator new(unsigned long) (/usr/lib/libasan.so.8+0x12d2a1) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x55be3b50620b in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/df/scratch.cpp:3
    … (кадри libc і _start обрізано)

SUMMARY: AddressSanitizer: double-free /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/m3/df/scratch.cpp:6 in main
```

Код виходу 1. Висновок: другий `delete` (рядок 6) віддає купі блок, який вона вже отримала в рядку 5, — ASan ловить це на самому `delete`, ще до того, як алокатор зіпсує свої списки.

Виправлення — один `delete` на один `new`; після нього `p = nullptr`, і випадковий другий `delete` стає безпечним (`delete nullptr` нічого не робить):

```cpp
#include <iostream>
int main() {
    int* p = new int{42};
    std::cout << *p << '\n';
    delete p;
    p = nullptr;
    delete p;                  // delete of nullptr does nothing
}
```

```txt
42
```

Звіту немає, код виходу 0.

## Карта пам'яті

Рамка — з ISA §2; у запасі підписані наші адреси, у купі — вказівник.

```txt
0x000 ┌─────────────────────────────┐
      │  CODE                       │  2048 байтів
      │  ваша програма; PC стартує  │  тут живуть інструкції
      │  саме звідси                │
0x800 ├─────────────────────────────┤
      │  DATA                       │  512 байтів
      │  рядки, масиви, лічильники  │
0xA00 ├─────────────────────────────┤
      │  VRAM   64 x 32, біт/піксель│  256 байтів  <- екран ЦЕ пам'ять
0xB00 ├─────────────────────────────┤
      │  (запас — ваш)              │  256 байтів
      │  0xB00 PORT_OUT  0xB01 TICKS│  0xB02 байт стану
0xC00 ├─────────────────────────────┤
      │  HEAP     ▼ до старших адрес│  768 байтів
      │                             │  ALLOC відрізає блоки вгору від 0xC00
      │  heap_ptr = наступний блок  │  FREE (0x72): список вільних блоків
0xF00 ├─────────────────────────────┤
      │  STACK    ▲ до молодших     │  256 байтів
      │                             │  SP стартує з 0xFFF і йде вниз
0xFFF └─────────────────────────────┘
```

| Регіон | Діапазон | Константа | З'являється в |
|---|---|---|---|
| Код | `0x000`–`0x7FF` | `CODE_LO`, `CODE_HI` | Lab 2 |
| Дані | `0x800`–`0x9FF` | `DATA_LO`, `DATA_HI` | Lab 3 |
| Відеопам'ять | `0xA00`–`0xAFF` | `VRAM_LO`, `VRAM_HI` | Lab 5 |
| Запас | `0xB00`–`0xBFF` | — | — |
| ↳ порт виводу | `0xB00` | `PORT_OUT` | Lab 5 (*opt*) |
| ↳ таймер | `0xB01` | `TICKS` | Lab 5 (*opt*) |
| ↳ байт стану (власне розширення) | `0xB02` | `STATUS_ADDR` | Lab 2 (з Lab 5 — `0xB02`) |
| **Купа** | `0xC00`–`0xEFF` | `HEAP_LO`, `HEAP_HI` | Lab 6 |
| Стек | `0xF00`–`0xFFF` | `STACK_LO`, `STACK_HI` | Lab 7 |

Константи в `src/memory.hpp` (`STACK_*` оголошені наперед, вживатиме Lab 07; `PORT_OUT`, `TICKS`, `STATUS_ADDR` — у `src/cpu.hpp`):

```cpp
const std::size_t CODE_LO = 0x000, CODE_HI = 0x7FF; // PC starts at CODE_LO (Lab 2)
const std::size_t DATA_LO = 0x800, DATA_HI = 0x9FF; // strings, arrays, counters (Lab 3)
const std::size_t VRAM_LO = 0xA00, VRAM_HI = 0xAFF; // 64x32 screen, 1 bit per pixel (Lab 5)
const std::size_t HEAP_LO = 0xC00, HEAP_HI = 0xEFF; // guest heap: ALLOC cuts blocks upwards (Lab 6)
const std::size_t STACK_LO = 0xF00, STACK_HI = 0xFFF; // SP starts at STACK_HI and goes down (Lab 7)
```

## Advanced

### Купа гостя: `ALLOC`

`CPU::heap_ptr` (`std::uint16_t`, стартує з `HEAP_LO`) — вершина купи; у `regs` його немає (це не регістр ISA), його видно в `heap`, `layout cpu` і `hostdump cpu`. `ALLOC` (`0x60`): блок влазить, якщо `heap_ptr + A <= HEAP_HI + 1` (блок може закінчитись рівно на `0xEFF`; рахується в `size_t`, без обрізання до 16 бітів). Влізло — `H = heap_ptr`, `heap_ptr += A`, `C = 0`; не влізло — `C = 1`, `H` і `heap_ptr` не змінюються. `Z`, `N`, `A`, `B` `ALLOC` не чіпає; `PC += 1` в обох випадках — це не помилка кроку, як `PLOT` за екраном. `A = 0` — буквально за формулою ISA: 0 байтів влазять завжди, `H = heap_ptr`, нічого не посувається.

```cpp
    case 0x6:
        // The guest heap (heap.cpp). Not fitting is no error of the step:
        // C = 1 and H stays. Z, N, A and B are never touched.
        switch (ins.op) {
        case Op::Alloc:
            hr = heap_alloc(cpu, cpu.a, block, flag);
            if (hr == HeapResult::broken_list)
                return StepResult::bad_heap;
            if (hr == HeapResult::ok)
                cpu.h = block;
            cpu.f.c = hr != HeapResult::ok;
            break;
        default:
            return StepResult::bad_opcode;
        }
        break;

    // Bump, as in the ISA. The block may end on HEAP_HI itself; counted in
    // size_t, so heap_ptr + n is never cut to 16 bits.
    if (static_cast<std::size_t>(cpu.heap_ptr) + n > HEAP_HI + 1)
        return HeapResult::no_room;
    at = cpu.heap_ptr;
    cpu.heap_ptr = static_cast<std::uint16_t>(cpu.heap_ptr + n);
    return HeapResult::ok;
```

Трасування з DEFENSE (T4): 16, 32, потім тричі 255:

```txt
адр.  байти       текст
0000  20 10       LOADI A, 0x10     ; 16
0002  60          ALLOC
0003  20 20       LOADI A, 0x20     ; 32
0005  60          ALLOC
0006  20 FF       LOADI A, 0xFF     ; 255
0008  60          ALLOC
0009  60          ALLOC
000A  60          ALLOC             ; не влізло
000B  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x10
ember> set 2 0x60
ember> set 3 0x20
ember> set 4 0x20
ember> set 5 0x60
ember> set 6 0x20
ember> set 7 0xFF
ember> set 8 0x60
ember> set 9 0x60
ember> set 10 0x60
ember> set 11 0x00
ember> trace on
ember> run
0000  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0000  Z=0 N=0 C=0
0002  60        ALLOC             -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
0003  20 20     LOADI A, 0x20     -> A= 32  B=  0  H=0x0c00  Z=0 N=0 C=0
0005  60        ALLOC             -> A= 32  B=  0  H=0x0c10  Z=0 N=0 C=0
0006  20 ff     LOADI A, 0xff     -> A=255  B=  0  H=0x0c10  Z=0 N=0 C=0
0008  60        ALLOC             -> A=255  B=  0  H=0x0c30  Z=0 N=0 C=0
0009  60        ALLOC             -> A=255  B=  0  H=0x0d2f  Z=0 N=0 C=0
000a  60        ALLOC             -> A=255  B=  0  H=0x0d2f  Z=0 N=0 C=1
000b  00        HALT              -> A=255  B=  0  H=0x0d2f  Z=0 N=0 C=1
halted after 9 steps
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0e2e, 558 used, 210 left
free list: empty
ember> quit
```

`0xE2E + 255 = 0xF2D > 0xF00` — п'ятий `ALLOC` не влазить: `C = 1`, `H` лишився `0x0D2F`, `heap_ptr` — `0x0E2E`.

### Дамп після двох виділень

Сам `ALLOC` пам'ять не пише, тож програма заповнює кожен блок через `H`: перший (4 байти) — `'A'`, другий (3 байти) — `'B'`:

```txt
адр.  байти       текст
0000  20 04       LOADI A, 0x04
0002  60          ALLOC             ; H = блок 1
0003  20 41       LOADI A, 0x41     ; 'A'
0005  2A          STORE [H], A
0006  2B          INCH
0007  2A          STORE [H], A
0008  2B          INCH
0009  2A          STORE [H], A
000A  2B          INCH
000B  2A          STORE [H], A
000C  20 03       LOADI A, 0x03
000E  60          ALLOC             ; H = блок 2
000F  20 42       LOADI A, 0x42     ; 'B'
0011  2A          STORE [H], A
0012  2B          INCH
0013  2A          STORE [H], A
0014  2B          INCH
0015  2A          STORE [H], A
0016  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x04
ember> set 2 0x60
ember> set 3 0x20
ember> set 4 0x41
ember> set 5 0x2A
ember> set 6 0x2B
ember> set 7 0x2A
ember> set 8 0x2B
ember> set 9 0x2A
ember> set 10 0x2B
ember> set 11 0x2A
ember> set 12 0x20
ember> set 13 0x03
ember> set 14 0x60
ember> set 15 0x20
ember> set 16 0x42
ember> set 17 0x2A
ember> set 18 0x2B
ember> set 19 0x2A
ember> set 20 0x2B
ember> set 21 0x2A
ember> set 22 0x00
ember> trace on
ember> run
0000  20 04     LOADI A, 0x04     -> A=  4  B=  0  H=0x0000  Z=0 N=0 C=0
0002  60        ALLOC             -> A=  4  B=  0  H=0x0c00  Z=0 N=0 C=0
0003  20 41     LOADI A, 0x41     -> A= 65  B=  0  H=0x0c00  Z=0 N=0 C=0
0005  2a        STORE [H], A      -> A= 65  B=  0  H=0x0c00  Z=0 N=0 C=0
0006  2b        INCH              -> A= 65  B=  0  H=0x0c01  Z=0 N=0 C=0
0007  2a        STORE [H], A      -> A= 65  B=  0  H=0x0c01  Z=0 N=0 C=0
0008  2b        INCH              -> A= 65  B=  0  H=0x0c02  Z=0 N=0 C=0
0009  2a        STORE [H], A      -> A= 65  B=  0  H=0x0c02  Z=0 N=0 C=0
000a  2b        INCH              -> A= 65  B=  0  H=0x0c03  Z=0 N=0 C=0
000b  2a        STORE [H], A      -> A= 65  B=  0  H=0x0c03  Z=0 N=0 C=0
000c  20 03     LOADI A, 0x03     -> A=  3  B=  0  H=0x0c03  Z=0 N=0 C=0
000e  60        ALLOC             -> A=  3  B=  0  H=0x0c04  Z=0 N=0 C=0
000f  20 42     LOADI A, 0x42     -> A= 66  B=  0  H=0x0c04  Z=0 N=0 C=0
0011  2a        STORE [H], A      -> A= 66  B=  0  H=0x0c04  Z=0 N=0 C=0
0012  2b        INCH              -> A= 66  B=  0  H=0x0c05  Z=0 N=0 C=0
0013  2a        STORE [H], A      -> A= 66  B=  0  H=0x0c05  Z=0 N=0 C=0
0014  2b        INCH              -> A= 66  B=  0  H=0x0c06  Z=0 N=0 C=0
0015  2a        STORE [H], A      -> A= 66  B=  0  H=0x0c06  Z=0 N=0 C=0
0016  00        HALT              -> A= 66  B=  0  H=0x0c06  Z=0 N=0 C=0
halted after 19 steps
ember> dump 0xC00 16
0c00  41 41 41 41 42 42 42 00 00 00 00 00 00 00 00 00  |AAAABBB.........|
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c07, 7 used, 761 left
free list: empty
ember> quit
```

`AAAABBB` — без дірки: `ALLOC` віддає `H = heap_ptr` і одразу `heap_ptr += A`, тож наступний блок починається рівно там, де закінчився попередній. Перед блоком немає заголовка (розміру ніде не записано), а байтам вирівнювання не потрібне — проміжку нізвідки взятись. Щоб став можливим `FREE`, довелось би десь зберігати розмір кожного блока (у заголовку перед ним — тоді дірка з'явилась би — або в самій інструкції `FREE`) і пам'ятати, які шматки вільні: список вільних блоків. Як я це зробив — у «Якщо встигаєте».

### `FREE` немає — і що це коштує

У ISA `FREE` немає: bump-алокатор уміє рухатись тільки вперед. Пам'ять, віддана `ALLOC`, не повертається до кінця програми (у `ember` — до перезапуску): програма, яка в циклі бере тимчасові блоки, рано чи пізно вичерпає 768 байтів і отримає `C = 1`, хоча майже всі її блоки давно не потрібні. Окремого об'єкта не звільнити, лише все разом. Натомість `ALLOC` — одне порівняння й одне додавання (O(1)), без метаданих і без фрагментації, з наперед відомим часом виконання; тому прошивки з правилами на кшталт MISRA C, де `malloc`/`free` заборонені, тримають саме такі пули, що тільки ростуть.

Справжній звіт про витік на Linux — у M3; тегований `Value` і `FREE` через список вільних блоків — нижче (зроблено обидва).

## Якщо встигаєте

### `FREE` через список вільних блоків

Власний опкод `FREE` (`0x72`, рядок — у «Власних розширеннях»). Рішення:

- **Розмір каже гість.** `FREE` повертає блок `[H, H + A)`, де `A` — той самий розмір, що просили в `ALLOC` (як `operator delete(void*, std::size_t)`). Заголовок із розміром перед блоком неможливий: ISA вимагає свіжі блоки без дірок (T4: 16 байтів на `0xC00` → наступний на `0xC10`). Ціна: гість, що збрехав про розмір, зіпсує купу, і `ember` не завжди це помітить.
- **Список — у пам'яті гостя, усередині звільнених блоків.** Перші 3 байти вільного блока — вузол: адреса наступного вільного блока (16 бітів, молодший уперед, `0` — кінець) і розмір цього. Голова — `CPU::free_head` (`0` — порожньо). Новий вузол стає в голову. Звідси мінімум: не верхній блок, менший за 3 байти, звільнити не можна (`C = 1`).
- **Верхній блок** (`H + A == heap_ptr`) не потрапляє в список: `heap_ptr` опускається до `H`, а за ним — через усі вільні блоки, що тепер закінчуються на `heap_ptr`. Звільнили все — `heap_ptr` знову `0xC00`.
- **`ALLOC` при непорожньому списку** спершу шукає там (first fit): вузол рівно на `A` байтів виймається зі списку, більший щонайменше на 3 байти віддає свій **хвіст** (вузол лишається на місці зі зменшеним розміром — посилань переписувати не треба). Інакше — bump за ISA. Програма без `0x72` бачить чистий bump.
- **Помилки — `C = 1`, нічого не змінено:** `A = 0`; блок не з зайнятої частини купи; блок перетинається з вільним (**повторне звільнення** — гостьовий аналог `attempting double-free`); не верхній і менший за 3 байти. `H` і `A` після `FREE` не змінюються: `H` тепер — завислий вказівник гостя.
- **Зіпсований список.** Гість, що пише у звільнений блок через старий `H`, псує вузол. Кожен обхід списку перевіряє вузли (адреса в зайнятій частині купи, розмір щонайменше 3 і не за `heap_ptr`, жоден вузол двічі — інакше петля); порушено — крок відмовляє: `free list is broken at 0x…; nothing done`, `run` зупиняється (як `free(): corrupted …` у glibc). Перевірка йде першою, до будь-яких змін, і для `ALLOC`, і для `FREE`.
- **Чого немає:** сусідні вільні блоки не зливаються — пам'ять може фрагментуватись (два вільні по 8 байтів не дадуть блок на 16).

Вузли читаються й пишуться через ті самі `guest_read`/`guest_write`, що й `LOAD`/`STORE` гостя:

```cpp
HeapResult heap_free(CPU& cpu, std::uint16_t at, Byte n, bool& top) {
    top = false;
    // The list first, as in ALLOC: a broken one is reported whatever the
    // block is, so the same FREE never stops one run and not another.
    std::uint16_t bad = 0;
    if (!heap_check(cpu, bad))
        return HeapResult::broken_list;
    if (n == 0 || at < HEAP_LO || at + n > cpu.heap_ptr)
        return HeapResult::bad_block;
    // A block that overlaps a free one was freed already (or never was a block).
    for (std::uint16_t node = cpu.free_head; node != 0; node = node_next(cpu, node)) {
        if (node < at + n && at < node + node_size(cpu, node))
            return HeapResult::already_free;
    }

    if (at + n == cpu.heap_ptr) {
        // The top block: heap_ptr comes down to it. Free blocks that now end
        // at heap_ptr are not between used ones any more: down past them too.
        top = true;
        cpu.heap_ptr = at;
        bool lowered = true;
        while (lowered) {
            lowered = false;
            for (std::uint16_t prev = 0, node = cpu.free_head; node != 0;
                 prev = node, node = node_next(cpu, node)) {
                if (node + node_size(cpu, node) == cpu.heap_ptr) {
                    take_out(cpu, prev, node);
                    cpu.heap_ptr = node;
                    lowered = true;
                    break;
                }
            }
        }
        return HeapResult::ok;
    }

    if (n < NODE_SIZE)
        return HeapResult::too_small;
    // Any other block: a node at its start, pushed on the head of the list.
    // The rest of its bytes stay as they were.
    set_next(cpu, at, cpu.free_head);
    set_size(cpu, at, n);
    cpu.free_head = at;
    return HeapResult::ok;
}
```

Звільнити, звільнити вдруге, взяти знову (точка зупину перед другим `FREE`, щоб показати вузол: `00 00` — кінець списку, `10` — 16 байтів):

```txt
адр.  байти       текст
0000  20 10       LOADI A, 0x10
0002  60          ALLOC             ; 0x0C00
0003  20 08       LOADI A, 0x08
0005  60          ALLOC             ; 0x0C10
0006  20 04       LOADI A, 0x04
0008  60          ALLOC             ; 0x0C18
0009  28 00 0C    LOADH H, 0x0C00
000C  20 10       LOADI A, 0x10
000E  72          FREE              ; [0x0C00, +16) -> список
000F  72          FREE              ; той самий блок ще раз
0010  60          ALLOC             ; A = 16: зі списку
0011  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x10
ember> set 2 0x60
ember> set 3 0x20
ember> set 4 0x08
ember> set 5 0x60
ember> set 6 0x20
ember> set 7 0x04
ember> set 8 0x60
ember> set 9 0x28
ember> set 10 0x00
ember> set 11 0x0C
ember> set 12 0x20
ember> set 13 0x10
ember> set 14 0x72
ember> set 15 0x72
ember> set 16 0x60
ember> set 17 0x00
ember> trace on
ember> b 0x0F
breakpoint at 0x000f
ember> run
0000  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0000  Z=0 N=0 C=0
0002  60        ALLOC             -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
0003  20 08     LOADI A, 0x08     -> A=  8  B=  0  H=0x0c00  Z=0 N=0 C=0
0005  60        ALLOC             -> A=  8  B=  0  H=0x0c10  Z=0 N=0 C=0
0006  20 04     LOADI A, 0x04     -> A=  4  B=  0  H=0x0c10  Z=0 N=0 C=0
0008  60        ALLOC             -> A=  4  B=  0  H=0x0c18  Z=0 N=0 C=0
0009  28 00 0c  LOADH H, 0x0c00   -> A=  4  B=  0  H=0x0c00  Z=0 N=0 C=0
000c  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
000e  72        FREE              -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
breakpoint at 0x000f
stopped after 9 steps
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c1c, 28 used, 740 left
free list: 0x0c00 (16 bytes) -> end
ember> dump 0xC00 16
0c00  00 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> run
000f  72        FREE              -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=1
0010  60        ALLOC             -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
0011  00        HALT              -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
halted after 3 steps
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c1c, 28 used, 740 left
free list: empty
ember> quit
```

Повторний `FREE` — `C = 1`, список не змінився; `ALLOC` на 16 узяв блок зі списку, і `heap_ptr` лишився `0x0C1C`. Це свідоме уточнення ISA: блок зі списку бере `H` з-під вершини, тож «вказівник купи посувається» стосується тільки bump.

Верхній блок опускає `heap_ptr`, а за ним — вільний блок `0x0C00`, що тепер закінчується на вершині:

```txt
адр.  байти       текст
0000  20 10       LOADI A, 0x10
0002  60          ALLOC             ; 0x0C00
0003  20 08       LOADI A, 0x08
0005  60          ALLOC             ; 0x0C10
0006  20 04       LOADI A, 0x04
0008  60          ALLOC             ; 0x0C18
0009  28 00 0C    LOADH H, 0x0C00
000C  20 10       LOADI A, 0x10
000E  72          FREE              ; -> список
000F  28 18 0C    LOADH H, 0x0C18
0012  20 04       LOADI A, 0x04
0014  72          FREE              ; верхній
0015  28 10 0C    LOADH H, 0x0C10
0018  20 08       LOADI A, 0x08
001A  72          FREE              ; верхній, за ним 0x0C00
001B  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x10
ember> set 2 0x60
ember> set 3 0x20
ember> set 4 0x08
ember> set 5 0x60
ember> set 6 0x20
ember> set 7 0x04
ember> set 8 0x60
ember> set 9 0x28
ember> set 10 0x00
ember> set 11 0x0C
ember> set 12 0x20
ember> set 13 0x10
ember> set 14 0x72
ember> set 15 0x28
ember> set 16 0x18
ember> set 17 0x0C
ember> set 18 0x20
ember> set 19 0x04
ember> set 20 0x72
ember> set 21 0x28
ember> set 22 0x10
ember> set 23 0x0C
ember> set 24 0x20
ember> set 25 0x08
ember> set 26 0x72
ember> set 27 0x00
ember> trace on
ember> run
0000  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0000  Z=0 N=0 C=0
0002  60        ALLOC             -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
0003  20 08     LOADI A, 0x08     -> A=  8  B=  0  H=0x0c00  Z=0 N=0 C=0
0005  60        ALLOC             -> A=  8  B=  0  H=0x0c10  Z=0 N=0 C=0
0006  20 04     LOADI A, 0x04     -> A=  4  B=  0  H=0x0c10  Z=0 N=0 C=0
0008  60        ALLOC             -> A=  4  B=  0  H=0x0c18  Z=0 N=0 C=0
0009  28 00 0c  LOADH H, 0x0c00   -> A=  4  B=  0  H=0x0c00  Z=0 N=0 C=0
000c  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
000e  72        FREE              -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
000f  28 18 0c  LOADH H, 0x0c18   -> A= 16  B=  0  H=0x0c18  Z=0 N=0 C=0
0012  20 04     LOADI A, 0x04     -> A=  4  B=  0  H=0x0c18  Z=0 N=0 C=0
0014  72        FREE              -> A=  4  B=  0  H=0x0c18  Z=0 N=0 C=0
0015  28 10 0c  LOADH H, 0x0c10   -> A=  4  B=  0  H=0x0c10  Z=0 N=0 C=0
0018  20 08     LOADI A, 0x08     -> A=  8  B=  0  H=0x0c10  Z=0 N=0 C=0
001a  72        FREE              -> A=  8  B=  0  H=0x0c10  Z=0 N=0 C=0
001b  00        HALT              -> A=  8  B=  0  H=0x0c10  Z=0 N=0 C=0
halted after 16 steps
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c00, 0 used, 768 left
free list: empty
ember> quit
```

Хвіст більшого вільного блока, потім запис у звільнений блок через старий `H` — гостьове читання/запис після звільнення:

```txt
адр.  байти       текст
0000  20 10       LOADI A, 0x10
0002  60          ALLOC             ; 0x0C00
0003  20 04       LOADI A, 0x04
0005  60          ALLOC             ; 0x0C10
0006  28 00 0C    LOADH H, 0x0C00
0009  20 10       LOADI A, 0x10
000B  72          FREE              ; -> список
000C  20 05       LOADI A, 0x05
000E  60          ALLOC             ; хвіст 0x0C00
000F  28 00 0C    LOADH H, 0x0C00
0012  20 FF       LOADI A, 0xFF
0014  2A          STORE [H], A      ; у звільнений блок
0015  20 01       LOADI A, 0x01
0017  60          ALLOC
0018  00          HALT
```

```txt
ember> set 0 0x20
ember> set 1 0x10
ember> set 2 0x60
ember> set 3 0x20
ember> set 4 0x04
ember> set 5 0x60
ember> set 6 0x28
ember> set 7 0x00
ember> set 8 0x0C
ember> set 9 0x20
ember> set 10 0x10
ember> set 11 0x72
ember> set 12 0x20
ember> set 13 0x05
ember> set 14 0x60
ember> set 15 0x28
ember> set 16 0x00
ember> set 17 0x0C
ember> set 18 0x20
ember> set 19 0xFF
ember> set 20 0x2A
ember> set 21 0x20
ember> set 22 0x01
ember> set 23 0x60
ember> set 24 0x00
ember> trace on
ember> run
0000  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0000  Z=0 N=0 C=0
0002  60        ALLOC             -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
0003  20 04     LOADI A, 0x04     -> A=  4  B=  0  H=0x0c00  Z=0 N=0 C=0
0005  60        ALLOC             -> A=  4  B=  0  H=0x0c10  Z=0 N=0 C=0
0006  28 00 0c  LOADH H, 0x0c00   -> A=  4  B=  0  H=0x0c00  Z=0 N=0 C=0
0009  20 10     LOADI A, 0x10     -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
000b  72        FREE              -> A= 16  B=  0  H=0x0c00  Z=0 N=0 C=0
000c  20 05     LOADI A, 0x05     -> A=  5  B=  0  H=0x0c00  Z=0 N=0 C=0
000e  60        ALLOC             -> A=  5  B=  0  H=0x0c0b  Z=0 N=0 C=0
000f  28 00 0c  LOADH H, 0x0c00   -> A=  5  B=  0  H=0x0c00  Z=0 N=0 C=0
0012  20 ff     LOADI A, 0xff     -> A=255  B=  0  H=0x0c00  Z=0 N=0 C=0
0014  2a        STORE [H], A      -> A=255  B=  0  H=0x0c00  Z=0 N=0 C=0
0015  20 01     LOADI A, 0x01     -> A=  1  B=  0  H=0x0c00  Z=0 N=0 C=0
free list is broken at 0x00ff (ALLOC at 0x0017); nothing done
stopped after 13 steps
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c14, 20 used, 748 left
free list: 0x0c00 (11 bytes) -> broken at 0x00ff
ember> dump 0xC00 16
0c00  ff 00 0b 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
ember> quit
```

Розщеплення: блок 16 байтів на `0xC00` віддав хвіст `0x0C0B..0x0C0F` (5 байтів), вузол лишився з розміром 11 (`0b`). Запис `0xFF` у байт `0x0C00` зробив посилання на наступний вузол `0x00FF` — `ALLOC` відмовився, нічого не змінивши, `run` зупинився.

Те саме з REPL — `alloc`/`free`/`heap` з помилками:

```txt
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c00, 0 used, 768 left
free list: empty
ember> alloc 16
allocated 16 bytes at 0x0c00 (bump), heap_ptr=0x0c10
ember> alloc 8
allocated 8 bytes at 0x0c10 (bump), heap_ptr=0x0c18
ember> free 0xC00 16
freed 16 bytes at 0x0c00 (free list)
ember> free 0xC00 16
not freed: already free; nothing done
ember> free 0xC04 4
not freed: already free; nothing done
ember> free 0xC10 2
not freed: too small for a list node (3 bytes); nothing done
ember> free 0xB00 4
not freed: not a used heap block; nothing done
ember> free 0xC00 0
not freed: not a used heap block; nothing done
ember> alloc 5
allocated 5 bytes at 0x0c0b (reused a free block), heap_ptr=0x0c18
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c18, 24 used, 744 left
free list: 0x0c00 (11 bytes) -> end
ember> free 0xC10 8
freed 8 bytes at 0x0c10 (top: heap_ptr=0x0c10)
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0c10, 16 used, 752 left
free list: 0x0c00 (11 bytes) -> end
ember> alloc 0
allocated 0 bytes at 0x0c10 (bump), heap_ptr=0x0c10
ember> alloc 255
allocated 255 bytes at 0x0c10 (bump), heap_ptr=0x0d0f
ember> alloc 255
allocated 255 bytes at 0x0d0f (bump), heap_ptr=0x0e0e
ember> alloc 255
255 bytes do not fit: heap_ptr=0x0e0e, 242 left; nothing done
ember> heap
heap 0x0c00-0x0eff: heap_ptr=0x0e0e, 526 used, 242 left
free list: 0x0c00 (11 bytes) -> end
ember> regs
PC=0x0000  A=0  B=0  H=0x0000  Z=0 N=0 C=0
ember> quit
```

`regs` наприкінці — нулі: хост працює як відладчик, регістри гостя не чіпає.

### Тегований `struct Value`: об'єднання проти тегованої структури

Даний рядок лаби `struct Value { enum class Kind { Byte, Addr, SpriteId }; uint16_t bits; };` оголошує тип `Kind`, але не поле цього типу: тега в байтах немає. Я лишив імена лаби й додав поле `kind`:

```cpp
// lab: struct Value { enum class Kind { Byte, Addr, SpriteId }; uint16_t bits; };
// That line declares the type Kind but no member of it: sizeof == 2 and no
// tag in the bytes. `kind` is the tag; ": std::uint8_t" keeps it one byte
// (an int would make sizeof 8).
struct Value {
    enum class Kind : std::uint8_t { Byte, Addr, SpriteId };
    Kind kind;          // the tag: which of the three `bits` means
    std::uint16_t bits; // a byte (0..255), a guest address or a sprite index
};
```

Три варіанти, `sizeof` на цій машині (GCC 16): як у лабі (без поля) — 2; з полем `Kind kind`, де `Kind` без типу-основи (`int`) — 8; наш (`enum class Kind : std::uint8_t`) — 4, з одним вирівнювальним байтом перед `bits`:

```cpp
#include <cstdint>
#include <iostream>
struct ValueLab { enum class Kind { Byte, Addr, SpriteId }; uint16_t bits; };
struct ValueInt { enum class Kind { Byte, Addr, SpriteId }; Kind kind; std::uint16_t bits; };
struct ValueOurs { enum class Kind : std::uint8_t { Byte, Addr, SpriteId }; Kind kind; std::uint16_t bits; };
int main() {
    std::cout << sizeof(ValueLab) << ' ' << sizeof(ValueInt) << ' ' << sizeof(ValueOurs) << '\n';
}
```

```txt
2 8 4
```

`make_value` перевіряє діапазон виду (байт 0..255, адреса 0..4095, спрайт 0..7), `print_value` — один `switch` за тегом. Ним друкують `value`, `sprite` і `sprites`:

```txt
ember> layout value
Value: sizeof=4 alignof=2
  off size field
    0    1 kind (Kind)
    1    1 (padding)
    2    2 bits
  fields 3 + padding 1 = 4
ember> value byte 65
byte 65: 65  0x41  0b01000001  'A'
ember> value addr 0xC00
addr 0x0c00: heap, byte 0x00
ember> value addr 0xB00
addr 0x0b00: port_out, byte 0x00
ember> value addr 0xB02
addr 0x0b02: status, byte 0x00
ember> sprite 1 62 1 1 -1
sprite 1: x=62 y=1 vx=1 vy=-1 alive
ember> value sprite 1
sprite 1: x=62 y=1 vx=1 vy=-1 alive
ember> value byte 300
a byte is 0..255, got 300
ember> value addr 5000
address 5000 is outside 0..4095
ember> value sprite 8
sprite index is 0..7, got 8
ember> value word 1
usage: value <byte|addr|sprite> <n>
ember> quit
```

**Об'єднання** (`union`) кладе всі варіанти в ті самі байти, а який із них там зараз, пам'ятає **програміст**: у байтах цього немає. У C++ читати не той член, що записали останнім, — невизначена поведінка, і UBSan її не ловить; GCC документує, що читання дає байти, тому на little-endian машині видно молодший байт адреси, а законний спосіб подивитись на байти — `std::memcpy`:

```cpp
#include <cstdint>
#include <cstring>
#include <iostream>
union Raw {                 // the same 2 bytes, two names
    std::uint16_t addr;
    std::uint8_t byte;
};
int main() {
    Raw r;
    r.addr = 0x0C41;        // the active member is now addr
    std::cout << sizeof(Raw) << ' ' << std::hex << int(r.byte) << '\n'; // UB in C++: byte is not active
    std::uint8_t b;
    std::memcpy(&b, &r.addr, 1); // the defined way to look at the bytes
    std::cout << int(b) << '\n';
}
```

```txt
2 41
41
```

**Тегована структура** тримає вид у самих байтах (`kind`): `print_value` не вгадує, а читає тег, і `value addr 0xC00` та `value byte 65` не сплутати. Ціна — байт тега плюс вирівнювання (4 замість 2).

### `std::unique_ptr<Sprite>`: `delete` живе в деструкторі

Хостовий `Sprite` у купі, сирим `new` без `delete` — витік рівно `sizeof(Sprite)` = 5 байтів:

```cpp
#include <cstdint>
#include <iostream>
struct Sprite {
    std::uint8_t x, y;
    std::int8_t  vx, vy;
    bool alive;
};
int main() {
    Sprite* p = new Sprite{10, 5, 1, 0, true};
    std::cout << int(p->x) << ' ' << sizeof(Sprite) << '\n';
}
```

```txt
10 5

=================================================================
==925654==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 5 byte(s) in 1 object(s) allocated from:
    #0 0x7fd8db92d2a1 in operator new(unsigned long) (/usr/lib/libasan.so.8+0x12d2a1) (BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)
    #1 0x5624784ac22b in main /tmp/claude-1000/-home-user-Projects-ember/e8eee81f-3d32-4df4-a5c8-a6f79bd51e2b/scratchpad/l6/x/raw/scratch.cpp:9
    … (кадри libc і _start обрізано)

SUMMARY: AddressSanitizer: 5 byte(s) leaked in 1 allocation(s).
```

Те саме через `std::make_unique` — `delete` викликає деструктор `unique_ptr`, коли `p` виходить з області видимості:

```cpp
#include <cstdint>
#include <iostream>
#include <memory>
struct Sprite {
    std::uint8_t x, y;
    std::int8_t  vx, vy;
    bool alive;
};
int main() {
    auto p = std::make_unique<Sprite>(Sprite{10, 5, 1, 0, true});
    std::cout << int(p->x) << ' ' << sizeof(Sprite) << '\n';
}   // here the unique_ptr dies, and its destructor calls delete
```

```txt
10 5
```

Звіту немає, код виходу 0. Обидва — тим самим рядком збірки, що в M3; у `src/` `unique_ptr` не з'являється (лаба просить подивитись, а не переписувати).

## Реалізовані рядки [ISA.uk.md](https://github.com/rmalkevy/Programming-Practice-Projects/blob/main/courses/programming-fundamentals/ISA.uk.md) (Лаба 6)

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x60` | `ALLOC` | 1 | C | виділити в купі `A` байтів: `H` стає адресою блока, вказівник купи посувається. Не влізло — виставити `C`, `H` не чіпати | 6 |

| Регіон | Діапазон | Константа | З'являється в |
|---|---|---|---|
| Купа | `0xC00`–`0xEFF` | `HEAP_LO`, `HEAP_HI` | Lab 6 |

## Власні розширення

| Опкод | Мнемоніка | Розмір | Прапорці | Що робить | Лаба |
|---|---|---|---|---|---|
| `0x72` | `FREE` | 1 | C | повернути в купу блок `[H, H + A)` (`A` — той самий розмір, що просили в `ALLOC`): верхній блок опускає вказівник купи, інший стає вузлом списку вільних блоків. Не блок купи, `A = 0`, уже вільний або менший за 3 байти не на вершині — виставити `C`, нічого не міняти | 6 |

Номер — з вільного діапазону `0x70`–`0xFF` (ISA §8); `0x61`–`0x6F` і `0x73` і далі лишаються невідомими опкодами. Уточнення рядка `ALLOC`: успіх — `C = 0`; `A = 0` — `H = heap_ptr`, нічого не посувається; при непорожньому списку вільних блоків `ALLOC` спершу бере звідти (first fit: блок рівно на `A` байтів або хвіст блока, більшого щонайменше на 3), і тоді `heap_ptr` не посувається; зіпсований список — помилка кроку, нічого не змінено. `JG`/`JL` (`0x70`/`0x71`) — як у Lab 04; байт стану — `0xB02`, як у Lab 05.
