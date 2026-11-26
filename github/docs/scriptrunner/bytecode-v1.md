# ScriptRunner Bytecode v1

Экспериментальный движок: исходник компилируется в байткод в вебе, МК
исполняет проверенный контейнер. Старый текстовый `ScriptRunner` не изменён и
остаётся основным, пока не включён флаг `ENABLE_BYTECODE_VM`.

## Быстрый старт

```bash
# 1. Прошивка с байткод-движком
cd /srv/vuesp-projects/vuesp
~/.platformio/penv/bin/pio run -e esp8266-bytecode -t upload --upload-port /dev/ttyUSB0

# 2. Web UI + LittleFS (страница /script-studio/)
cd vue && npm run build-web
cd ..
~/.platformio/penv/bin/pio run -e esp8266-bytecode -t buildfs
~/.platformio/penv/bin/pio run -e esp8266-bytecode -t uploadfs --upload-port /dev/ttyUSB0
```

Открыть `http://<device>/script-studio/`. Авторизация — та же, что у остальных
страниц устройства.

Флаг:

```cpp
// src/const/bytecode.h
#ifndef ENABLE_BYTECODE_VM
#define ENABLE_BYTECODE_VM 0
#endif
```

`[env:esp8266-bytecode]` включает флаг через `build_flags`. Обычный
`[env:esp8266]` остаётся без байткод-движка.

## Язык

Синтаксис похож на JavaScript, но переменные — фиксированные слоты:

```js
$u0 = 0;
$s0 = 'ready';

while ($u0 < 5) {
  $u0 = $u0 + 1;
  if ($u0 == 3) {
    continue;
  }
  log('step', $u0);
  wait(100ms);
}

$display = $s0 + ' ' + $u0;   // DataProvider
$p13 = $u0 > 3 ? 255 : 0;     // порт

on('btn_c') {
  log('gpio', $e0, 'value', $e1, 'count', $e2);
  if ($e1 == 1) {
    $p13 = 255;
  } else {
    $p13 = 0;
  }
}
```

Поддерживается:

- `$uN`, `$iN`, `$fN`, `$sN`, `$aN`, `$pN`, `$eN`, `$cN`;
- `$name` через `DataProvider` (в прошивке — `$display`);
- арифметика, битовые операции, сравнения, `&&`, `||`, тернарный `?:`,
  скобки, стандартный приоритет операторов;
- `if/else`, `while`, `break`, `continue`;
- `log`, `wait`, `call`, `set`, `get`, `len`, `chr`, `ord`;
- внешние функции (`fade`, `udp`, `http` и платовые);
- обработчики `on('event')`, `return`;
- строки в одинарных/двойных кавычках, экранирование;
- массивы `[1, 2, 3]` и индексация `$a0[0]`;
- комментарии `//` и `/* */`.

`wait()` нормализуется в миллисекунды: `wait(1s)` → `wait(1000ms)`.
`&&`/`||` в v1 вычисляют оба операнда (без short-circuit).

## Компиляция и декомпиляция

В вебе:

- **Compile** — исходник → `.vbc`, diagnostics, disassembly, hex, размер и
  глубина стека;
- **Upload & Run** — сохранить в LittleFS и запустить;
- **Load from device** — скачать `.vbc` и декомпилировать в канонический
  исходник;
- **Download .vbc** — сохранить контейнер на диск.

CLI:

```bash
node scripts/compile-script.mjs source.js out.vbc
```

Канонический кодогенератор детерминирован: одинаковый исходник и capability
profile дают одинаковый контейнер. Декомпилятор восстанавливает управляющие
блоки по block table, а выражения — симуляцией стека.

## Контейнер VBC1

```
0   4  "VBC1"
4   1  version
5   1  flags
6   2  header_size
8   2  code_len
10  2  const_count
12  2  block_count
14  2  handler_count
16  2  entry_offset
18  2  stack_max
20  4  source_hash
24  4  crc32
28  ... const pool
...     code
...     block table (kind, start, middle, end)
...     handler table (event_id, name_const, entry, end)
```

- CRC32 считается по всему контейнеру с обнулённым полем CRC.
- Константы: `INT32`, `UINT32`, `FLOAT32`, `STRING`, `NAME`, `ARRAY`.
- Инструкции: 1 байт opcode, little-endian операнды, absolute u16 jump.
- Верификатор проверяет CRC, границы, jump-цели, стек, переменные, константы,
  функции и события до исполнения.

Полный список опкодов — `src/runner/bytecode/opcodes.yaml`; C++/TS таблицы
генерируются `scripts/generate-opcodes.mjs`.

## Хранение

v1: `/www/scripts/<id>.vbc`, по файлу на скрипт. Запись идёт через `.tmp` →
`remove` → `rename`. В RAM держится кэш из 4 контейнеров; неактивные скрипты
вытесняются.

Единый `/www/scripts.bc` с TOC — запланированное развитие; текущий HTTP API
от этого не зависит.

## HTTP API

Все маршруты требуют ту же авторизацию, что и остальные страницы.

| Метод | Путь | Назначение |
|---|---|---|
| GET | `/script/capabilities` | capability manifest: VM, лимиты, функции, события, `dataVars` |
| GET | `/script/list` | список сохранённых `.vbc` |
| GET | `/script/status` | active, scripts, последняя ошибка |
| GET | `/script/bytecode?id=N` | скачать контейнер |
| POST | `/script/bytecode?id=N&run=1` | загрузить бинарный контейнер (тело запроса) |
| DELETE | `/script/bytecode?id=N` | удалить скрипт |
| POST | `/script/run?id=N` | запустить |
| POST | `/script/stop?id=N` | остановить |
| POST | `/script/stop-all` | остановить все |
| POST | `/script/event?name=btn_c&count=3&p0=4&p1=1&p2=2` | диагностически отправить событие |

Пример:

```bash
curl -u admin:admin \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @out.vbc \
  'http://<device>/script/bytecode?id=1&run=1'
```

## Лимиты v1

- до 15 сохранённых скриптов; 2 в RAM-кэше по умолчанию
  (`VBC_CACHE_SLOTS`, переопределяется при сборке);
- до 1024 байт кода на скрипт;
- стек до 64 значений;
- 32 инструкции или 300 µs на контекст за проход, 3 ms общий бюджет;
- `wait` не блокирует loop;
- ошибка останавливает только свой контекст и попадает в `/script/status`.

На ESP8266 сборка с флагом занимает около 73.8% RAM (60 440 / 81 920) и 42.7%
flash; обычная сборка не меняется (57.9% RAM).

## Тесты

```bash
# TS compiler/decompiler
cd vue && npm run test:script

# C++ reader/verifier/VM
cd ..
g++ -std=c++11 -I src -I src/tests -o /tmp/test_bytecode \
  src/tests/test_bytecode.cpp \
  src/runner/bytecode/BytecodeFormat.cpp \
  src/runner/bytecode/BytecodeInstruction.cpp \
  src/runner/bytecode/BytecodeVerifier.cpp \
  src/runner/bytecode/BytecodeVM.cpp \
  src/tests/arduino_stub.cpp
/tmp/test_bytecode
```

## Не входит в v1

- пользовательские функции с локальными переменными (v1.5);
- рекурсия и замыкания;
- short-circuit `&&`/`||`;
- динамический индекс порта;
- единый `/www/scripts.bc` с TOC.
