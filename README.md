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
