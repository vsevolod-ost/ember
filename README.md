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
