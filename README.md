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

## M1 - вітання і prompt

```txt
ember 0.1 - 4096 bytes of memory you can see. Type `help`.
ember>
```

## M2 - порожня коробка

```txt
ember> dump
0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
0020  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
```

## M4 - навмисні поломки

### 1. `inc` і загортання: unsigned wrap vs signed UB

`inc <addr>` читає байт, додає одиницю й пише назад — `b + 1` загортається за модулем 256:

```txt
ember> set 0 255
ember> get 0
255  0xff  0b11111111  '.'
ember> inc 0
ember> get 0
0  0x0  0b00000000  '.'
```

`255 + 1` дало `0`, а не `256` — байт **загорнувся**. Це визначена поведінка `std::uint8_t`. Той самий приклад із notes §2, зібраний з UBSan:

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
scratch.cpp:9:7: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
0
-2147483648
```

`uint8_t` загорнувся визначено, `int` дав UB — і його спіймав UBSan.

### 2. `(0.1 + 0.2 == 0.3) == false`

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

`0.1`, `0.2` і `0.3` не мають точного двійкового запису - кожен округлюється до найближчого `double`.

### 3. `"AB\0"` у дампі

```txt
ember> set 0 65
ember> set 1 66
ember> set 2 0
ember> dump
0000  41 42 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |AB..............|
```

У пам'яті лежить C-рядок `"AB"`: два символи й нуль-термінатор.

### 4. Прибрані `{}` - неініціалізована пам'ять

```txt
ember> dump
0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
```

Дамп усе одно був нульовим: ОС віддає процесу свіжі, ще не займані сторінки пам'яті, але це просто збіг обставин.

## `sizeof` на моїй машині

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

1. **Стала ширина.** `sizeof(std::uint8_t) == 1` гарантовано скрізь; `sizeof(int)` — ні (таблиця `sizeof` вище).
2. **Визначене переповнення.** Беззнакове загортання (`255 + 1 == 0`) - визначена модульна арифметика; переповнення знакового `int` - невизначена поведінка.

## Advanced

`set16 <addr> <value>` пише 16-бітне значення little-endian, атомарно по межах (обидві клітинки в коробці, інакше не пише жодної).

```txt
ember> set16 0 0x1234
ember> dump
0000  34 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |4...............|
```

Молодший байт `0x34` ліг першим, за нижчою адресою — цей ноутбук (x86-64) little-endian. Big-endian машина поклала б `12 34`.

`.clang-format` застосовано до всього `src/`: `clang-format -i src/*.cpp src/*.hpp`.

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
