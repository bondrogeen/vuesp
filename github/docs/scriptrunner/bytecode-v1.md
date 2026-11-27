# ScriptRunner Bytecode v1

Единственный движок ScriptRunner: исходник компилируется в байткод в вебе,
МК исполняет проверенный контейнер. Текстовый раннер удалён.

## Быстрый старт

```bash
# 1. Прошивка
cd ~/github/pets/vuesp-projects/vuesp
~/.platformio/penv/bin/pio run -e esp8266 -t upload --upload-port /dev/ttyUSB0

# 2. Web UI + LittleFS (страница /script/)
cd vue && npm run build-web
cd ..
~/.platformio/penv/bin/pio run -e esp8266 -t buildfs
~/.platformio/penv/bin/pio run -e esp8266 -t uploadfs --upload-port /dev/ttyUSB0
```

Открыть `http://<device>/script/`. Авторизация — та же, что у остальных
страниц устройства. Движок включён во всех сборках.

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

Вызов другого скрипта — по имени:

```js
call('blink');
call('display', 1, 2);
```

- имя — строковый литерал, lowercase, 1–16 символов;
- до 4 аргументов, они попадают в `$c0…$c3` целевого скрипта;
- возврата нет, `call` — statement;
- вызывающий продолжает работу;
- цель загружается из пакета лениво;
- рекурсия запрещена, глубина вызовов — `VBC_MAX_CALL_DEPTH` (default 4);
- при нехватке слотов кэша — ошибка `NO_CACHE`.

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

v1: единый `/www/scripts.bc`.

```text
Header 14 байт:
  magic "VSP1" | version | count | flags | reserved | tocSize | tocCrc

TOC: count × 25 байт:
  name[16] | flags | codeOff u16 | codeLen u16 | codeCrc u32

Code blobs
```

- имена lowercase, 1–16 символов, уникальные;
- `boot` ищется по точному имени; если его нет — автозапуска нет;
- запись идёт через `scripts.bc.tmp` → `rename`, с `.bak` для отката;
- TOC и каждый code blob защищены CRC;
- старые `/www/scripts/<id>.vbc` не мигрируются и не используются;
- `/www/scripts.txt` больше не используется.

В RAM держатся TOC, карта `name → внутренний id` и кэш контейнеров
(`VBC_CACHE_SLOTS`, default 3). Неактивные скрипты вытесняются.

## HTTP API

Все маршруты требуют ту же авторизацию, что и остальные страницы.

| Метод | Путь | Назначение |
|---|---|---|
| GET | `/script/capabilities` | capability manifest: VM, лимиты, функции, события, `dataVars` |
| GET | `/script/list` | список сохранённых скриптов `{name,size,boot,flags}` |
| GET | `/script/status` | active, scripts, saved, cacheSlots, packError, последняя ошибка |
| GET | `/script/bytecode?name=boot` | скачать контейнер |
| POST | `/script/bytecode?name=boot&run=1` | загрузить контейнер (тело запроса) |
| DELETE | `/script/bytecode?name=boot` | удалить скрипт |
| POST | `/script/run?name=boot` | запустить |
| POST | `/script/stop?name=boot` | остановить |
| POST | `/script/stop-all` | остановить все |
| POST | `/script/event?name=btn_c&count=3&p0=4&p1=1&p2=2` | диагностически отправить событие |

Пример:

```bash
curl -u admin:admin \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @out.vbc \
  'http://<device>/script/bytecode?name=boot&run=1'
```

## Лимиты v1

- до 15 сохранённых скриптов; 3 в RAM-кэше по умолчанию
  (`VBC_CACHE_SLOTS`, переопределяется при сборке);
- до 1024 байт кода на скрипт;
- стек до 64 значений;
- до 4 аргументов у `call('name', ...)`, без возврата;
- глубина вызовов до 4 (`VBC_MAX_CALL_DEPTH`);
- рекурсия запрещена;
- 32 инструкции или 300 µs на контекст за проход, 3 ms общий бюджет;
- `wait` не блокирует loop;
- ошибка останавливает только свой контекст и попадает в `/script/status`.

На ESP8266 сборка занимает около 61.7% RAM (50 528 / 81 920) и 41.1% flash.

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

# C++ pack format
g++ -std=c++11 -I src -I src/tests -o /tmp/test_bytecode_pack \
  src/tests/test_bytecode_pack.cpp \
  src/runner/bytecode/BytecodeFormat.cpp \
  src/runner/bytecode/BytecodePack.cpp
/tmp/test_bytecode_pack
```

## Не входит в v1

- пользовательские функции с локальными переменными (v1.5);
- возврат значений из `call`;
- замыкания;
- short-circuit `&&`/`||`;
- динамический индекс порта.
