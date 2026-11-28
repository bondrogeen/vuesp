# ScriptRunner Bytecode v1

Версия: 6.0.0
Обновлено: 2026-11-28

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

on('btn_c', (gpio, value, count) => {
  log('gpio', gpio, 'value', value, 'count', count);
  if (value == 1) {
    $p13 = 255;
  } else {
    $p13 = 0;
  }
});
```

Поддерживается:

- `$uN`, `$iN`, `$fN`, `$sN`, `$aN`, `$pN`, `$eN`, `$cN`;
- `$name` через `DataProvider` (в прошивке — `$display`);
- арифметика, битовые операции, сравнения, `&&`, `||`, тернарный `?:`,
  скобки, стандартный приоритет операторов;
- `if/else`, `while`, `break`, `continue`;
- `log`, `wait`, `call`, `set`, `get`, `len`, `chr`, `ord`;
- внешние функции (`fade`, `udp`, `http` и платовые);
- обработчики `on('event', (args) => { ... })`, `return`;
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
- **Import** — загрузить `.js`, `.txt` или `.vjs` в редактор;
- **Download** — сохранить текущий исходник как `.js`.

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
(`VBC_CACHE_SLOTS`, default 6). Неактивные скрипты вытесняются.

## HTTP API

Все маршруты требуют ту же авторизацию, что и остальные страницы. Через HTTP
идут только редкие операции; команды и статус — по WebSocket.

| Метод | Путь | Назначение |
|---|---|---|
| GET | `/vm/capabilities` | JSON: VM, лимиты, слоты, функции, события |
| GET | `/vm/list` | JSON: `[{"name":"boot","state":10}]` |
| GET | `/vm/bytecode?name=boot` | скачать контейнер (потоково) |
| POST | `/vm/bytecode?name=boot&request=1&run=1` | загрузить контейнер (тело запроса) |

Загрузка возвращает `202 Accepted` и `requestId`; результат сохранения
приходит сообщением `KEY_SCRIPTSTATUS`. Тело запроса пишется сразу во временный
файл, без 1-КБ буфера в RAM; проверка контейнера выполняется в `loop()`.

Пример:

```bash
curl -u admin:admin \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @out.vbc \
  'http://<device>/vm/bytecode?name=boot&request=1&run=1'
```

## WebSocket

Тот же бинарный канал `/esp`:

- `KEY_SCRIPTCMD` (`ScriptCmd`) — команды:
  `STATUS`, `RUN`, `STOP`, `STOP_ALL`, `REMOVE`;
  поля `action`, `flags`, `requestId`, `name[16]` (до 15 символов + NUL).
- `KEY_SCRIPTSTATUS` (`ScriptStatus`) — результат и состояние:
  `result`, `state` (active/loaded/error/boot), `packError`, `error`,
  `errorPc`, `name[16]` (до 15 символов + NUL).

Статус приходит после каждой команды и после завершения HTTP-загрузки;
`requestId` связывает ответ с загрузкой. Отдельного `/vm/status` нет.

## Аргументы событий

Предпочтительный вид — именованные параметры обработчика:

```js
on('btn_c', (gpio, value, count) => {
  log('click', gpio, value, count);
});
```

- параметры мапятся на `$e0…$e3`;
- видны только внутри этого обработчика;
- только чтение;
- у каждого вызова свои значения (они хранятся в контексте);
- `call()` не наследует аргументы обработчика.

| Событие | Параметры |
|---|---|
| `btn` | `gpio, value` |
| `btn_c` | `gpio, value, count` |
| `btn_l` | `gpio, value` |
| `btn_r` | `gpio, value` |
| `time` | `hours, minutes` |
| `modbus` | нет |

Старый низкоуровневый вид `on('btn_c') { log($e0, $e1, $e2); }` тоже
поддерживается. Переменные `$u/$i/$f/$s/$a` локальны для загруженного
скрипта: одинаковые имена в разных скриптах — разные переменные. `$p`
остаются общими, потому что порт один на всех.

## Лимиты v1

Все лимиты движка собраны в `vuesp.config.yaml` и попадают в прошивку через
`src/generated/vuespConfig.h` и `src/generated/scriptManifest.h`. Профили кэша
задаются через `build_flags` и переопределяют значения из манифеста:

- до 15 сохранённых скриптов; 6 в RAM-кэше по умолчанию
  (`VBC_CACHE_SLOTS`, переопределяется при сборке);
- до 512 байт контейнера на скрипт по умолчанию
  (`VBC_MAX_CODE_BYTES`, профили 256/512/1024);
- профили кэша: `basic` 256×12, `standard` 512×6, `pro` 1024×3
  (произведение слотов на размер не больше 12 КБ);
- слоты: `$u0..$u9`, `$i0..$i9`, `$f0..$f4`, `$s0..$s1`, `$a0..$a4`,
  `$p0..$p39`, `$e0..$e3`, `$c0..$c3`; `$pN` — номер GPIO, для `N > 15`
  компилятор использует wide-опкоды `LOAD_PORT`/`STORE_PORT`;
- стек до 64 значений;
- пул локальных переменных `VBC_VAR_POOL_BYTES` (по умолчанию 3072 B,
  минимум 2048 B) делится между загруженными скриптами;
- до 4 аргументов у `call('name', ...)`, без возврата;
- глубина вызовов до 4 (`VBC_MAX_CALL_DEPTH`);
- рекурсия запрещена;
- 32 инструкции или 300 µs на контекст за проход, 3 ms общий бюджет;
- `wait` не блокирует loop;
- ошибка останавливает только свой контекст и попадает в `KEY_SCRIPTSTATUS`.

## Добавление события, функции или лимита

Единственный источник — `vuesp.config.yaml`. `npm run config` генерирует C++
заголовки и веб-манифест.

1. Добавьте запись в `kScriptEvents`:

   ```cpp
   {6, "time", 2, "hours,minutes", "device uptime clock"},
   ```

   - `id` — уникальный и стабильный навсегда: он хранится в `.vbc` и
     по нему рантайм находит обработчик;
   - `name` — контракт скрипта (`on('time', ...)`);
   - `params` — число аргументов, до `MAX_EVENT_PARAMS`;
   - `args` — канонические имена для редактора и декомпилятора; в `.vbc`
     не сохраняются, поэтому переименование безопасно.

2. При необходимости вызовите событие из `loop()` (не из ISR):

   ```cpp
   bytecodeService.vm().emitEvent("time", 2, hours, minutes);
   ```

3. Сгенерируйте веб-манифест:

   ```bash
   cd vue && npm run manifest
   ```

   Команда обновляет `vue/src/generated/scriptManifest.ts`:
   `SCRIPT_EVENTS`, `SCRIPT_EVENT_ARG_NAMES`, `SCRIPT_FUNCTIONS`,
   `SCRIPT_LIMITS`. `npm run build-web` запускает её автоматически.

4. Проверьте, что сгенерированный файл не устарел:

   ```bash
   cd vue && npm run manifest:check
   ```

Функции добавляются так же в `kScriptFunctions` (`id`, `name`, `min`, `max`,
`ret`, описание), лимиты — в `ScriptLimits`/`kScriptLimits`. Реализацию
эмиттера события или внешней функции всё равно нужно написать руками:
манифест описывает контракт, а не поведение.

Веб — надмножество всех сборок: генератор всегда включает Modbus-записи.
На конкретном устройстве авторитетен ответ `/vm/capabilities`.

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
