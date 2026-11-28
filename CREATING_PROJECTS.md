# Создание нового проекта на основе Vuesp

Практическое руководство по созданию device-проектов на базе [`vuesp`](https://github.com/bondrogeen/vuesp). Описывает путь от `vuesp-create` до прошивки, web UI, dashboard, документации и проверки на железе.

## 1. Выбор базы

Новый проект — это отдельный репозиторий в `vuesp-devices`, например:

```text
vuesp-devices/
  vuesp.kc868-a6
  vuesp.kc868-a16
  vuesp.lcd-2432S024C
```

Быстрый старт:

```bash
npx vuesp-create@latest my-project
```

CLI клонирует шаблон вместе с git-историей, позволяет выбрать плату, задаёт имя проекта и при необходимости ставит зависимости. `--no-fork` создаёт независимую копию без общего шаблона.

Правила:

- имя проекта — `vuesp.<board>`, например `vuesp.kc868-a16`;
- не создавать проект внутри другого device-проекта;
- не запускать `sync` поверх грязного рабочего дерева;
- перед обновлением шаблона сохранить `git diff` и копию рабочего дерева;
- для нового проекта предпочтительнее **чистая копия** `vuesp`, а затем ручной перенос платового кода, а не `sync` поверх старой архитектуры.

Обновление шаблона в существующем проекте:

```bash
npm run sync:check
npm run sync
```

После `sync` обязательно проверить `git diff`: шаблон может затронуть `platformio.ini`, `vue`, `README`, CI и платовые файлы.

## 2. Структура проекта

```text
vuesp.<board>/
├── vuesp.config.yaml  # плата, модули, сеть, лимиты, web-композиция
├── config/
│   └── modules/       # дескрипторы модулей
├── src/
│   ├── config/        # ConfigStore
│   ├── const/         # protocol.h и общие константы
│   ├── generated/     # generated config/manifests
│   ├── device/        # платовые данные, I/O, ADC, датчики
│   ├── diag/          # boot log и loop-метрики
│   ├── discovery/     # UDP discovery
│   ├── eeprom/        # Settings (Wi-Fi, auth, сеть)
│   ├── files/         # LittleFS, OTA, info
│   ├── gpio/          # порты Vuesp
│   ├── init/          # Wi-Fi/Ethernet, загрузка настроек
│   ├── modbus/        # при наличии RS485
│   ├── pages/         # recovery
│   ├── runner/        # ScriptRunner
│   ├── setup/         # serial setup
│   ├── tasks/         # WS-задачи
│   ├── tests/
│   ├── utils/
│   ├── main.cpp
├── vue/
│   ├── json/          # dashboard и locales
│   ├── scripts/       # protocol-and-json
│   ├── src/           # pages, components, stores, generated
│   └── public/
├── data/              # LittleFS: www, tmp
├── github/            # скриншоты и изображения
├── platformio.ini     # shell
├── platformio.generated.ini # default_envs, [common], build_src_filter
├── package.json
├── README.md
├── DOCS.md
└── .vuesp.json
```

Не добавлять в git `build/`, `data/www`, `node_modules`, `*.tsbuildinfo`, локальные `docs/` и `AGENTS.md`, если они исключены.

## 3. Плата, модули и PlatformIO

Плата и модули задаются в `vuesp.config.yaml`:

```yaml
project:
  board: esp32

features:
  script: true
  modbus: true
  serial: false
  discovery: true
  dashboard: true
```

`project.board` выбирает `default_envs` в `platformio.generated.ini`. Команда
`pio run` без `-e` собирает выбранную плату; `pio run -e <env>` переопределяет.

`platformio.ini` остаётся shell-файлом:

- оставить только реальные env платы;
- указать `board`, `board_build.filesystem`, `lib_deps`, `build_flags`;
- `[common]` (`firmware_version`, `device_name`), `default_envs` и
  `build_src_filter` генерируются из YAML;
- для OTA проверить размер app-слота и запас flash;
- CI-матрицу синхронизировать со списком env.

Платовые пины:

- штатные таблицы портов платы — в `src/gpio/gpio.cpp`;
- переопределения — `hardware.pins` в `vuesp.config.yaml`;
- overrides применяются до чтения `ConfigStore`, сохранённые настройки портов
  имеют приоритет;
- I2C/SPI/UART, Ethernet PHY, реле/MOSFET/PCF8574, ADC и кнопки, которых нет в
  таблице платы, по-прежнему задаются в коде платы.

Не смешивать пины разных ревизий платы. Если есть R/C/N-варианты, зафиксировать
в `DOCS.md`, для какой именно ревизии проект.

## 4. Протокол

`src/const/protocol.h` — единственный источник структуры обмена.

Порядок:

1. Добавить/изменить `key`, `cmd`, `Settings`, `Info`, `Device` и платовые структуры.
2. Не менять порядок полей без необходимости: web/прошивка собираются вместе, но старые клиенты могут кэшироваться.
3. Поднять версию `CONFIG_*_VERSION` / `SETTINGS_VERSION`, если меняется размер или смысл сохраняемых данных.
4. Выполнить:

```bash
cd vue
npm run protocol
```

5. Проверить `src/const/protocol.generated.h`: статические assert'ы размера и смещений должны сойтись.
6. Добавить поля в дефолтные объекты web-стора (`vue/src/stores/WebSocketStore.ts`) и типы, если генератор их не создал автоматически.

Правила:

- конфиг и телеметрию не смешивать в одной структуре без причины;
- настройки, которые меняются вместе, держать в `Settings`;
- состояние устройства (входы, выходы, ADC, температура) — в `Device` или отдельной телеметрической структуре;
- не управлять группой портов тремя отдельными командами, если нужна атомарность: лучше одно поле/одна структура и один WS-кадр;
- `get`/`set` dashboard-выражения — только для простых преобразований, не для разбора битовых масок и hex-цветов.

## 5. Прошивка

### ConfigStore

Постоянные настройки портов/устройства/профилей — в `ConfigStore` (`/tmp/config.bin`). Для каждой записи:

- ключ;
- версия;
- фиксированный размер;
- миграция или явный сброс.

### Settings

Wi-Fi, авторизация, Ethernet, discovery — в `Settings` (EEPROM). При изменении размера:

- поднять `SETTINGS_VERSION`;
- решить, нужна ли миграция;
- если миграции нет — предупредить в `DOCS.md`, что Wi-Fi/сеть надо настроить заново.

### Порты

`src/gpio/gpio.cpp` — список `ports[]` для страницы GPIO:

- только безопасные для пользователя пины;
- занятые (дисплей, SD, тач, Ethernet, PSRAM/flash) не выводить;
- для каждого пина задать `mode`, `list`, `disabled`, `interrupt`;
- входы с подтяжкой и активным низким уровнем проверять на реальной плате;
- не включать выход по умолчанию на пине, который может управлять внешней нагрузкой.

### Платовое устройство

`src/device/device.cpp`:

- чтение/запись дискретных входов/выходов;
- ADC;
- события входов;
- ScriptRunner-функции (`input`, `output`, при необходимости `modbus`);
- периодическая отправка `Device` только при изменении или с разумным интервалом.

Не копировать платовый код A6/A16 целиком: различаются количество портов, PCF-адреса, полярность, Ethernet, RTC/DAC/RS232/LoRa.

### Сеть

`src/init/init.cpp`:

- Wi-Fi;
- Ethernet (если есть): `ETH.begin`, DHCP или `ETH.config`;
- обновление `Info` (link, speed, IP);
- после сохранения сетевых настроек — перезагрузка.

Если Ethernet и Wi-Fi работают одновременно, проверить оба IP и discovery.

## 6. Web UI

### Зависимости

`vue/package.json`:

- `vuesp-components` — npm-версия или локальная file-зависимость;
- `vuesp-struct@2`;
- скрипт `protocol` (`node ./scripts/protocol-and-json.js`).

Локальная зависимость:

```json
"vuesp-components": "file:../../../vuesp-components"
```

После правок библиотеки:

```bash
cd ../../../vuesp-components
npm run build
```

Если package использует file-зависимость и в его `node_modules` есть свой Vue, в `tsconfig.app.json` может понадобиться маппинг `vue`/`@vue/*` на корневой `node_modules` проекта — иначе появятся конфликтующие типы.

### Страницы

- файловый роутинг: `vue/src/pages/`;
- device-страницы: `vue/src/pages/device/[page].vue`;
- меню: генерируется из `vuesp.config.yaml` (`web.menu`) в `vue/json/menu.json`;
- локали: `vue/json/locales/en.json`, `ru.json`; состав языков — `web.locales`;
- dashboard: доступные типы — `web.dashboard.cards`, экземпляры —
  `vue/json/dashboard.json`.

### Dashboard

Для плат с большим числом I/O использовать паттерн A6/A16:

- входы — `type: "button"`, `opts.disabled: 1`, `get` по биту;
- выходы — `type: "button"` с `set` и `command: CMD.SET`;
- ADC — `type: "info"`;
- Ethernet/Modbus — отдельные info/сервисные карточки.

Пример выхода:

```json
{
  "id": "device.output.1",
  "name": "output.1",
  "type": "button",
  "icon": "bulb",
  "args": ["device.output", "device"],
  "get": "(a & 1)",
  "getTo": "v ? 'ON' : 'OFF'",
  "set": "({...b, output: v ? (b.output | 1) : (b.output & ~1), command: CMD.SET})"
}
```

Если у платы есть отдельная страница I/O, сначала уточнить, нужна ли она: на A6/A16 управление и состояние удобнее держать карточками dashboard, без дублирующей страницы.

### Scripts и suggestions

`vue/json/suggestions.json` дополняет автодополнение редактора Scripts и содержит **только платовые** функции и события. Глобальные вещи уже лежат в `vuesp-components`:

- `vuesp-components/src/components/script/editor/suggestions.ts` → `DEFAULT_SUGGESTIONS`;
- там `wait`, `log`, `len`, `get`, `set`, `fade`, `udp`, `http`, `on`, `btn`, `btn_c/l/r`, переменные `$u/$i/$f/$s/$a/$e/$c/$p`;
- дублировать их в проектах не нужно: редактор объединяет `DEFAULT_SUGGESTIONS` и `suggestions.json`.

Формат записи:

```json
{
  "label": "output",
  "type": "func",
  "insert": "output(${0}, ${1})",
  "detail": "output(pin:number, value?:number) => value:number",
  "description": "Read or write an output. 1 turns it on, 0 turns it off."
}
```

- `type`: `func`, `event` или `var`;
- `label` — то, что видит пользователь в списке;
- `insert` — шаблон вставки с `${0}`, `${1}`;
- `detail` — сигнатура;
- `description` — короткое пояснение, что делает функция.

После правки:

```bash
cd vue
npm run protocol
```

`protocol-and-json.js` собирает `vue/json/*.json` в `vue/public/default.json`, откуда suggestions читает веб-интерфейс. Без этого шага изменения не попадут в сборку.

Стартовые примеры лежат в `vue/src/lib/script/examples.ts`. После `build-web`
они доступны в студии `/script/`, а пользовательский pack сохраняется в
LittleFS как `/www/scripts.bc`.

События и внешние функции описываются в `vuesp.config.yaml`:

```yaml
script:
  events:
    - id: 6
      name: time
      args: [hours, minutes]
  functions:
    - id: 6
      name: temp
      minArgs: 0
      maxArgs: 0
      ret: float
```

После правки:

```bash
cd vue
npm run config
npm run build-web
```

Эмиттер события и реализацию внешней функции всё равно нужно написать в
прошивке. Лимит внешних функций — `script.maxExternalFunctions` в YAML.

Полярность и индексы фиксировать в `DOCS.md` проекта. Например, у A6
`output(0,0)` включает реле, а у A16 `output(1,1)` включает выход — одинаковый
код на разных платах ведёт себя по-разному.

### UI-правила

- карточки — `VCard`; кнопки — `VButton`; иконки — `VIcon`;
- иконочные кнопки — с `title` и `aria-label`;
- отступы — по шкале Tailwind 4 px;
- не собирать карточки вручную из `bg-*`, `border-*`, `p-*`;
- все состояния: loading, empty, error, disabled;
- проверять светлую и тёмную темы, мобильную ширину.

## 7. Проверка

### Чистая сборка

```bash
cd vue
npm ci
npm run protocol
npm run typecheck
npm run lint
npm run build-web

cd ..
~/.platformio/penv/bin/pio run -e esp32
~/.platformio/penv/bin/pio run -e esp32 -t buildfs
```

### Mock и скриншоты

```bash
cd vue
npm run dev:mock
npm run screen
```

### Железо

Проверить на реальной плате:

- входы по одному;
- выходы по одному, полярность и безопасность нагрузки;
- ADC с известным напряжением;
- Ethernet: link, DHCP, static, перезагрузка;
- Wi-Fi + Ethernet одновременно;
- RS485/Modbus, если есть;
- OTA;
- сохранение настроек после перезагрузки;
- серийный лог загрузки и `VUESP:PING`/`VUESP:IO`, если добавлены.

### Что фиксировать в отчёте

- версия прошивки и web;
- env/board;
- размер flash/RAM;
- что проверено на железе, что осталось;
- известные ограничения и отсутствующие компоненты.

## 8. Документация проекта

Минимум:

- `README.md` — что это, возможности, сборка, ссылки;
- `DOCS.md` — характеристики, распиновка, интерфейсы, API, ScriptRunner, ограничения, источники;
- `.vuesp.json` — метаданные проекта;
- скриншоты — `github/images/screen/`;
- источники — официальная схема/мануал, ESPHome, форумы.

Не писать в документации неподтверждённые пины и характеристики. Если ревизия платы неясна — указать это и дать способ проверки.

## 9. Git и релиз

- не коммитить `build/`, `data/www`, `node_modules`, секреты и локальные заметки;
- коммит и пуш делает пользователь, если не попросил иное;
- CI: frontend (typecheck/lint/build), firmware (актуальные env), ScriptRunner tests, артефакты;
- версия в `vuesp.config.yaml` (`project.firmwareVersion`) и `vue/package.json` должна совпадать;
- тег/релиз — только по команде пользователя.

## 10. Чек-лист нового проекта

- [ ] Проект создан через `vuesp-create` или чистую копию `vuesp`.
- [ ] Имя `vuesp.<board>`, версия 5.0.0.
- [ ] В `platformio.ini` только реальные env.
- [ ] Пины и ревизия платы зафиксированы в `hardware.pins`/`gpio.cpp` и `DOCS.md`.
- [ ] `protocol.h` обновлён, `npm run protocol` выполнен.
- [ ] `ConfigStore`/`Settings` версии подняты.
- [ ] Платовые порты и устройство не конфликтуют с display/SD/touch/сетью.
- [ ] Dashboard отражает все пользовательские I/O и ADC.
- [ ] `vue/json/suggestions.json` содержит только платовые функции и события, без глобальных `fade`/`udp`/`http`.
- [ ] `npm run config:check` и `npm run protocol` выполнены после правки YAML и suggestions.
- [ ] `vue/src/lib/script/examples.ts` содержит рабочие примеры с корректной полярностью.
- [ ] Ethernet/Modbus/RS485 проверены, если есть.
- [ ] `npm ci`, config:check, typecheck, lint, build-web, pio run, buildfs — успешно.
- [ ] Прошивка и FS залиты, железо проверено.
- [ ] README/DOCS актуальны, секреты не добавлены.
- [ ] Старый проект/резервная копия сохранены.

## 11. Типичные ошибки

- `sync` поверх грязного дерева без backup;
- копирование платового кода A6 в A16 без адаптации пинов/PCF/полярности;
- три отдельных `KEY.PORT` для RGB вместо одной атомарной команды;
- `get`/`set` dashboard-выражения с разбором hex/битовых масок;
- `ethEnabled`/`ethDhcp` в `Settings`, но `ETH.config` всегда статический;
- забыть `npm run protocol` после изменения `protocol.h`;
- file-зависимость `vuesp-components` без `npm run build` библиотеки;
- держать `data/www` и `build` в git;
- документация обещает RTC/DAC/RS232 на плате, где их нет;
- отдельная страница I/O, когда вся серия использует dashboard-карточки;
- дублирование глобальных suggestions (`fade`, `udp`, `http`, `btn*`) в каждом проекте;
- устаревшие примеры с `$v0` вместо `$u0`;
- примеры, которые при `boot` сразу включают выходы или реле.
