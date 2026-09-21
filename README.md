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

## M1 — `sizeof` на цій машині (64-bit Linux)

```cpp
#include <iostream>
#include <cstdint>
int main() {
    std::cout << sizeof(char) << ' ' << sizeof(int) << ' '
              << sizeof(float) << ' ' << sizeof(double) << ' '
              << sizeof(void*) << '\n';
    std::cout << sizeof(std::uint8_t) << ' ' << sizeof(std::uint16_t) << '\n';
}
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

Типовий результат для 64-бітної машини (`1 4 4 8 8` і `1 2`) — і саме
такий вивід я отримав. `sizeof(int)` і `sizeof(void*)` залежать від
платформи й ABI (стандарт гарантує лише мінімальну ширину); `std::uint8_t`
і `std::uint16_t` — рівно 1 і 2 байти **на будь-якій** платформі. Тому
клітинка `ember` — `std::uint8_t`, а не `int`: ширина клітинки має бути
однаковою скрізь, а не "чотири байти на моєму ноутбуці й хтозна-скільки
деінде".

## Символ — це число в костюмі (notes §4)

```cpp
#include <iostream>
int main() {
    std::cout << 65 << ' ' << 0x41 << ' '
              << static_cast<int>('A') << ' '
              << static_cast<char>(65) << '\n';
}
```

```txt
65 65 65 A
```

`65`, `0x41`, `static_cast<int>('A')` — це одне й те саме число, записане
трьома способами; `static_cast<char>(65)` показує ті самі біти як символ.
Локальний офлайн-еквівалент бонусного "Compiler Explorer" для `int x = 65;`
(без виходу в мережу — той самий ефект дає `-S`, асемблер для цієї ж
машини):

```txt
$ c++ -std=c++17 -O0 -S -masm=intel scratch.cpp -o scratch.s
mov    DWORD PTR -4[rbp], 65
mov    eax, DWORD PTR -4[rbp]
```

Перший рядок — інструкція, яка кладе безпосереднє значення `65` у
клітинку на стеку (компілятор вписав літерал прямо в код інструкції);
другий читає його назад у регістр `eax`. Той самий біт-патерн `01000001`
далі можна прочитати як число, hex, bin або символ — це і є весь фокус
Lab 1.

## Чому клітинка `ember` — `std::uint8_t`, а не `int`

Дві причини, обидві видно вище:

1. **Стала ширина.** `sizeof(std::uint8_t) == 1` гарантовано скрізь;
   `sizeof(int)` — ні (таблиця `sizeof` вище). Клітинка VM повинна важити
   рівно один байт на кожній машині, назавжди.
2. **Визначене переповнення.** Беззнакове загортання (`255 + 1 == 0` для
   `uint8_t`) — визначена модульна арифметика; переповнення **знакового**
   `int` — невизначена поведінка (доведено нижче, M4 §1, разом зі звітом
   UBSan). Пам'ять і майбутній ALU (Lab 2+) мають загортатися
   передбачувано, а не залежати від того, зловить це санітайзер чи ні.

## M4 — навмисні поломки (докази з термінала)

### 1. `inc` і загортання: unsigned wrap vs signed UB

Додано `inc <addr>` у диспетчер `main.cpp` (три рядки: прочитати байт,
додати одиницю, записати назад — `mem_set` бере `Byte`, тому `b + 1`
загортається за модулем 256, коли `b == 255`):

```txt
ember> set 0 255
ember> get 0
255  0xff  0b11111111  '.'
ember> inc 0
ember> get 0
0  0x0  0b00000000  '.'
```

`255 + 1` дало `0`, а не `256` — байт **загорнувся**. Це визначена
поведінка `std::uint8_t`. Контрастний дослід — той самий приклад із notes
§2, зібраний з UBSan:

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
$ c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined scratch.cpp -o scratch && ./scratch
scratch.cpp:9:7: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
0
-2147483648
```

Різниця: `uint8_t` загортається мовчки й передбачувано (модульна
арифметика, саме тому клітинки `ember` — `uint8_t`), а `int` того самого
розміру дає **невизначену поведінку** — UBSan ловить її й друкує звіт,
а без санітайзера програма мовчки видала б `-2147483648`, ніби так і
треба.

### 2. `0.1 + 0.2 == 0.3` — це `false`

```cpp
#include <iostream>
int main() {
    std::cout << (0.1 + 0.2) << '\n';
    std::cout << std::boolalpha << (0.1 + 0.2 == 0.3) << '\n';
}
```

```txt
$ c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined scratch.cpp -o scratch && ./scratch
0.3
false
```

`0.1`, `0.2` і `0.3` не мають точного двійкового запису — кожен
округлюється до найближчого `double`. Друк округлює суму назад до
`0.3` на вигляд, але біти під капотом інші, тому порівняння на
рівність — `false`. Це не баг компілятора: десяткові дроби взагалі не
адреса й не лічильник, і саме тому для них не варто використовувати
`==`.

### 3. `"AB\0"` у дампі

```txt
ember> set 0 65
ember> set 1 66
ember> set 2 0
ember> dump
0000  41 42 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |AB..............|
```

У пам'яті лежить C-рядок `"AB"`: два символи й нуль-термінатор одразу
за ними. ASCII-колонка показує `AB`, а решта — крапки, бо `0x00` не
друкується. У Lab 5, коли рядки стануть явними, цей нуль-байт і буде
межею рядка.

### 4. Прибрані `{}` — неініціалізована пам'ять

Прибрав `{}` з `Byte data[MEM_SIZE]{}` у `memory.hpp`, перезібрав і
зробив `dump` одразу після старту:

```txt
ember> dump
0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |................|
...усі 4096 байтів — нулі...
```

На цій машині дамп *усе одно* був нульовим: `Memory` — локальна
змінна на стеку в `main()`, а ОС віддає процесу свіжі, ще не займані
сторінки, які вже занулені нею самою — не ініціалізатором `{}`. Це
**збіг обставин, а не гарантія**: `{}` — контракт мови ("ця пам'ть
буде нулем"), нуль від ОС — випадковість реалізації, на яку не можна
покладатися. Щоб довести, що без `{}` там справді сміття, а не
"завжди нуль", я окремим сценарієм спершу забруднив ту саму ділянку
стека (записав туди `0xAA` в іншій функції), а потім оголосив масив
без `{}` у наступному виклику — на тому самому місці стека:

```cpp
Byte junk[MEM_SIZE];
std::memset(junk, 0xAA, sizeof(junk));   // "брудна" стекова пам'ять
// ...пізніше, в іншому виклику:
Byte data[MEM_SIZE];                      // без {} — те саме місце стека
```

```txt
first 16 bytes: aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa
non-zero byte count: 4096 / 4096
```

Компілятор навіть попереджає про це сам: `warning: 'data' may be used
uninitialized [-Wmaybe-uninitialized]`. Тобто без `{}` дамп показав би
сміття щоразу, коли ця ділянка стека раніше щось зберігала (вкладені
виклики, рекурсія, попередній кадр функції) — просто на голому старті
процесу цього не видно. **Дужки повернуто на місце** — читати
неініціалізовану пам'ять це UB, і цей курс UB не здає.

## Advanced — `set16` і порядок байтів

Додано `set16 <addr> <value>` у диспетчер `main.cpp`: перевіряє, що
значення влазить у 16 біт (`0..0xFFFF`) і що **обидві** клітинки
(`addr` і `addr+1`) в межах коробки — інакше не записує жодного байта
(атомарно, як і `set`) — і кладе значення в little-endian порядку.

```txt
ember> set16 0 0x1234
ember> dump
0000  34 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |4...............|
```

Куди подівся молодший байт `0x34`? Він ліг **першим**, за нижчою
адресою (0), а старший `0x12` — за адресою 1. Цей ноутбук (x86-64) —
little-endian: багатобайтові числа зберігаються "молодшим байтом
уперед". Big-endian машина поклала б `12 34`.

Межу перевірено окремо — команда не записує частину значення, якщо
другий байт не влазить у коробку:

```txt
ember> set16 4095 1
address 4095 is outside 0..4095
```

`.clang-format` (`BasedOnStyle: LLVM`, 4 пробіли, 100 колонок,
`PointerAlignment: Left` — той самий файл, що йшов зі скелетом) застосовано
до всього `src/`: `clang-format -i src/*.cpp src/*.hpp`.

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
