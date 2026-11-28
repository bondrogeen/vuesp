# Vuesp project configuration — спецификация

Статус: implemented (phase 1–3 + constructor preview). Документ описывает текущую архитектуру и границы следующих этапов.

Реализовано: `vuesp.config.yaml`, модульные дескрипторы, генерация C++/TS, web-композиция, firmware `build_src_filter`, `default_envs`, `[common]`, overrides пинов, `config/modules.generated.json`, `config/project.resolved.json` и `config:preview`.

Не реализовано: web-конструктор, чтение модульных дескрипторов внешним `vuesp-create`, полная генерация `platformio.ini`, генерация экземпляров `dashboard.json`.

## Практическая инструкция

### Главное правило

Все пользовательские настройки живут в одном файле в корне проекта:

```text
vuesp.config.yaml
```

Порядок работы:

```bash
cd vue

npm run config         # сгенерировать C++/TS/меню/фильтры из YAML
npm run config:check   # проверить, что generated-файлы актуальны
npm run config:preview # посмотреть итоговый resolved JSON
```

Дальше — сборка:

```bash
npm run build-web      # только web UI
npm run build          # web + прошивка
```

Или только прошивка:

```bash
cd ..
pio run
```

`pio run` без `-e` собирает плату, выбранную в `project.board`. Другой env можно указать явно:

```bash
pio run -e esp32
```

### 1. Что где менять

| Что нужно | Где менять |
|---|---|
| Плата | `project.board` |
| Версия прошивки | `project.firmwareVersion` |
| Имя устройства | `project.deviceName` |
| Включить/выключить модуль | `features.*` |
| Wi-Fi, IP, auth | `network.*` |
| Уровень логов | `logging.level` |
| Лимиты скриптов | `script.*` |
| События скриптов | `script.events` |
| Внешние функции скриптов | `script.functions` |
| Переопределить пин | `hardware.pins` |
| Пункты меню | `web.menu` |
| Базовые страницы | `web.pages` |
| Типы dashboard-карточек | `web.dashboard.cards` |
| Языки | `web.locales` |

### 2. Сменить плату

```yaml
project:
  board: esp32
```

Допустимые значения:

```text
esp8266
esp32
pico32
esp32-c3-devkitm-1
```

После изменения:

```bash
cd vue
npm run config
cd ..
pio run
```

Что произойдёт:

- `platformio.generated.ini` получит `default_envs = esp32`;
- `pio run` без `-e` соберёт ESP32;
- `pio run -e esp8266` всё ещё можно использовать для другой платы.

Если `project.board: esp8266`, а `features.modbus: true`, генератор выдаст ошибку: Modbus на ESP8266 не поддерживается.

### 3. Сменить версию и имя устройства

```yaml
project:
  firmwareVersion: "5.0.13"
  deviceName: "vuesp."
```

Что произойдёт:

- `firmwareVersion` попадёт в generated `[common]` и в `DEF_DEVICE_FIRMWARE`;
- `deviceName` попадёт в `DEF_DEVICE_NAME`;
- web UI в demo-режиме тоже возьмёт версию из YAML.

`project.name` — это имя проекта, а не имя устройства. Для прошивки используй `deviceName`.

### 4. Включить или выключить модуль

```yaml
features:
  script: true
  modbus: false
  serial: true
  discovery: true
  dashboard: true
```

После изменения:

```bash
cd vue
npm run config
npm run build-web
cd ..
pio run
```

| Модуль | Если выключить |
|---|---|
| `script` | `src/runner` не компилируется, `/vm/*` отключаются, страница `/script/` и `scriptStudio`-локали исчезают |
| `modbus` | `src/modbus` не компилируется, `/device/modbus/` исчезает, `modbus`-локаль исчезает |
| `serial` | страница `/serial/`, меню и локаль исчезают; прошивка не меняется |
| `discovery` | `src/discovery` не компилируется, `DEF_DISCOVERY=0`, карточка Discovery скрывается в настройках |
| `dashboard` | страница `/dashboard/` и меню исчезают |

Файлы физически не удаляются. Модуль просто исключается из сборки и веб-композиции.

### 5. Сменить Wi-Fi, IP, auth, discovery

```yaml
network:
  wifi:
    dhcp: true
    ip: 192.168.1.12
    subnet: 255.255.255.0
    gateway: 192.168.1.1
    dns: 8.8.8.8
    ssid: ""
    password: ""
  auth:
    enabled: true
    login: admin
    password: admin
  discovery:
    enabled: true
    interval: 5
    port: 12345
```

Это **дефолты для сборки**, а не runtime-настройки. Если на устройстве уже сохранены настройки через UI, они имеют приоритет.

После изменения:

```bash
cd vue
npm run config
cd ..
pio run
```

### 6. Сменить уровень логов

```yaml
logging:
  level: 2
```

| Уровень | Что пишет |
|---|---|
| `0` | тихо |
| `1` | ошибки и boot |
| `2` | события и жизненный цикл скриптов |
| `3` | подробно, включая порты и data |

Для CI можно переопределить флагом:

```bash
pio run -e esp8266 -D VUESP_LOG_LEVEL=0
```

### 7. Сменить профиль и лимиты скриптов

```yaml
project:
  buildProfile: standard
```

| Профиль | `maxCodeBytes` | `cacheSlots` |
|---|---|---|
| `basic` | 256 | 12 |
| `standard` | 512 | 6 |
| `pro` | 1024 | 3 |

Лимиты можно задать явно:

```yaml
script:
  maxCodeBytes: 512
  cacheSlots: 6
  stackSize: 64
  scratchBytes: 256
  maxStringLen: 32
  maxArrayLen: 64
  maxEventParams: 4
```

Важно:

- `maxCodeBytes` — только `256`, `512` или `1024`;
- `cacheSlots` — от 1 до 15;
- `cacheSlots * maxCodeBytes` не больше 12 КиБ.

Если в `script` заданы явные `maxCodeBytes`/`cacheSlots`, они переопределяют профиль. Если хочешь чистые значения профиля — удали эти два поля из `script`.

Полный список лимитов:

```yaml
script:
  maxCodeBytes: 512
  cacheSlots: 6
  maxSavedScripts: 15
  maxScripts: 15
  maxContexts: 15
  stackSize: 64
  scratchBytes: 256
  maxCallDepth: 4
  vars:
    u: 10
    i: 10
    f: 5
    s: 2
    a: 5
    p: 40
    e: 4
    c: 4
  maxStringLen: 32
  maxArrayLen: 64
  maxPwmValue: 255
  maxExternalFunctions: 16
  maxFunctionParams: 5
  maxCallParams: 4
  maxEventParams: 4
  instrPerTick: 32
  usPerTick: 300
  updateBudgetUs: 3000
```

После изменения:

```bash
cd vue
npm run config
cd ..
pio run
```

### 8. Добавить событие скрипта

Пример: событие `time` с двумя аргументами.

```yaml
script:
  events:
    - id: 6
      name: time
      args: [hours, minutes]
      description: "device uptime clock"
```

Правила:

- `id` уникален и не меняется после публикации;
- `name` — lowercase, стабильное имя;
- `args` — до 4 аргументов;
- `requires: modbus` — если событие относится к Modbus.

После изменения:

```bash
cd vue
npm run config
npm run build-web
cd ..
pio run
```

В студии появится:

```js
on('time', (hours, minutes) => {
  log('time', hours, minutes);
});
```

Но **эмиттер события всё равно пишется в C++**. Например, в `loop()`:

```cpp
bytecodeService.vm().emitEvent("time", 2, hours, minutes);
```

Не вызывай эмиттер из ISR.

### 9. Добавить внешнюю функцию скрипта

```yaml
script:
  functions:
    - id: 6
      name: temp
      minArgs: 0
      maxArgs: 0
      ret: float
      description: "read temperature"
```

Правила:

- `id` уникален и стабилен;
- `minArgs <= maxArgs`;
- `ret` — `int`, `uint`, `float`, `string`, `array` или `none`;
- `requires: modbus` — для Modbus-функций.

После изменения:

```bash
cd vue
npm run config
npm run build-web
cd ..
pio run
```

Саму функцию и её регистрацию всё равно нужно написать в прошивке, например:

```cpp
bytecodeService.vm().registerFunction(6, tempHandler);
```

### 10. Переопределить пин

```yaml
hardware:
  pins:
    - id: led
      gpio: 13
      mode: pwm
      value: 255
      interrupt: change
      button: true
```

| Поле | Значение |
|---|---|
| `id` | человекочитаемое имя |
| `gpio` | номер GPIO |
| `mode` | `input`, `input_pullup`, `output`, `output_open_drain`, `pwm`, `onewire`, `adc`, `dac` |
| `value` | начальное значение, необязательно |
| `interrupt` | `off`, `rising`, `falling`, `change` |
| `disabled` | `true/false` |
| `button` | `true/false`, включает обработку кнопки |

Важно:

- это **overrides** поверх штатной таблицы портов платы;
- применяются при старте **до** чтения `ConfigStore`;
- сохранённые на устройстве настройки портов имеют приоритет;
- если такого GPIO нет в таблице платы, override игнорируется.

Если на устройстве уже сохранены порты, YAML-изменение может не проявиться. Тогда нужно сбросить конфигурацию портов или изменить их через UI.

### 11. Меню, страницы, dashboard, языки

```yaml
web:
  menu:
    auto: true
    extra:
      - name: menu.custom
        icon: star
        path: /custom/
  pages:
    gpio: true
    settings: true
    storage: true
  dashboard:
    cards:
      - button
      - date
      - dimmer
      - info
      - input
      - list
      - chart
  locales:
    - ru
    - en
```

Что происходит:

- `menu.auto: true` собирает базовое меню из включённых модулей;
- `menu.extra` добавляет свои пункты;
- `pages` управляет базовыми service-страницами;
- `dashboard.cards` задаёт доступные типы карточек;
- выключенные типы карточек вырезаются из `default.json`;
- `locales` задаёт языки.

Сами экземпляры dashboard-карточек остаются в `vue/json/dashboard.json`. YAML управляет только доступными типами.

### 12. Что генерируется и что нельзя править руками

После `npm run config` появляются/обновляются:

```text
src/generated/vuespConfig.h
src/generated/scriptManifest.h
src/generated/gpioConfig.h
config/modules.generated.json
config/project.resolved.json
platformio.generated.ini
vue/src/generated/vuespConfig.ts
vue/src/generated/modules.ts
vue/src/generated/servicePages.ts
vue/src/generated/dashboardCards.ts
vue/src/generated/dashboardTypes.json
vue/src/generated/pages.json
vue/src/generated/localeSections.json
vue/json/menu.json
vuesp.config.schema.json
```

Эти файлы **не редактируются вручную**. Их перезапишет генератор.

Менять нужно только:

```text
vuesp.config.yaml
config/modules/*.yaml        # если добавляешь новый модуль
vue/json/dashboard.json      # контент карточек
vue/json/locales/*.json      # тексты локалей
```

### 13. Быстрая шпаргалка

| Хочу | Меняю | Запускаю |
|---|---|---|
| Другую плату | `project.board` | `npm run config`, `pio run` |
| Другой IP | `network.wifi.ip` | `npm run config`, `pio run` |
| Выключить Modbus | `features.modbus: false` | `npm run config`, `npm run build-web`, `pio run` |
| Добавить событие | `script.events` | `npm run config`, `npm run build-web` |
| Добавить функцию | `script.functions` | `npm run config`, `npm run build-web` |
| Изменить лимит кэша | `script.cacheSlots` | `npm run config`, `pio run` |
| Переопределить пин | `hardware.pins` | `npm run config`, `pio run` |
| Добавить пункт меню | `web.menu.extra` | `npm run config`, `npm run build-web` |
| Убрать тип карточки | `web.dashboard.cards` | `npm run config`, `npm run build-web` |
| Посмотреть итоговый конфиг | — | `npm run config:preview` |

### 14. Частые ошибки

**Изменил YAML, а ничего не поменялось.**
Нужно запустить `cd vue && npm run config`. `pio run` сам YAML не читает.

**`config:check` говорит, что файлы устарели.**
Запусти `npm run config`.

**Modbus не включается на ESP8266.**
Это запрещено. Выбери `esp32`, `pico32` или `esp32-c3-devkitm-1`.

**Пин не применился.**
Скорее всего, на устройстве уже сохранены порты в `ConfigStore`. Сохранённые настройки имеют приоритет. Сбрось порты или измени их через UI.

**Страница выключенного модуля всё ещё открывается.**
Для верхнеуровневых страниц (`/script/`, `/serial/`, `/dashboard/`) route исключается. Для service-страниц (`/device/modbus/`) показывается `pageNotFound`.

**Изменил generated-файл, и изменения пропали.**
Так и должно быть. Правь `vuesp.config.yaml` и запускай `npm run config`.

### 15. Границы текущей версии

Пока не реализовано:

- web-конструктор с формой и preview;
- чтение `config/modules/*.yaml` внешним `vuesp-create`;
- полная генерация `platformio.ini`;
- генерация экземпляров `dashboard.json`;
- полная генерация таблицы портов платы — сейчас только overrides.

## 1. Зафиксированные решения

| Решение | Значение |
|---|---|
| Формат пользовательского файла | YAML |
| Объём | все пользовательские настройки проекта |
| Расположение | корень репозитория |
| Generated-файлы | генерируются, вручную не редактируются |
| Режим конструктора | конфигурирует текущий шаблон |

## 2. Цели

1. Один понятный вход для пользователя: `vuesp.config.yaml`.
2. Один источник истины для прошивки, веба, документации и конструктора.
3. Валидация до сборки: диапазоны, зависимости, конфликты пинов, стабильные id.
4. Управление веб-частью: включение/выключение модулей, страниц, пунктов меню, dashboard-карточек и локалей.
5. Основа для веб-конструктора: машиночитаемая схема, каталог модулей и компонентов, preview изменений.
6. Воспроизводимость: одинаковый YAML даёт одинаковые generated-файлы и сборку.
7. Совместимость с текущим шаблоном и device-проектами.

## 3. Границы

### Входит в YAML

- плата, профиль сборки, имя проекта;
- включённые модули;
- пины и шины;
- сетевые дефолты, auth, discovery;
- уровень логов;
- лимиты скриптового движка;
- события и внешние функции скриптов;
- состав web UI: страницы, меню, dashboard-карточки, локали;
- флаги и параметры модулей.

### Не входит в YAML

- wire-структуры и id сообщений из `src/const/protocol.h`;
- размеры массивов внутри wire-структур;
- секреты и реальные IP устройства (только дефолты);
- тексты локалей и содержимое dashboard как контент;
- код Vue-компонентов и C++-модулей;
- SDK- и board-константы.

`protocol.h` остаётся ручным контрактом. Его enum'ы и struct-метаданные могут экспортироваться в TypeScript, но не редактируются из конструктора.

## 4. Термины

- **Модуль** — функциональный блок: `script`, `modbus`, `serial`, `discovery`, `dashboard`.
- **Страница** — route и Vue-страница.
- **Service page** — страница внутри `/device/:page`, например `settings`, `gpio`, `modbus`.
- **Компонент** — элемент каталога: dashboard-карточка, service-блок, widget.
- **Экземпляр** — выбранный пользователем компонент с настройками.
- **Generated** — файл, который создаётся генератором и не редактируется вручную.

## 5. Файлы и каталоги

```text
vuesp.config.yaml              # единственный пользовательский файл
vuesp.config.schema.json       # генерируемая JSON Schema
config/
  modules/
    script.yaml                # дескриптор модуля
    modbus.yaml
    serial.yaml
    discovery.yaml
    dashboard.yaml
  components/
    dashboard.yaml             # каталог dashboard-карточек
config/modules.generated.json  # каталог модулей для конструктора
config/project.resolved.json   # разрешённый проект для preview
platformio.generated.ini       # default_envs и build_src_filter
scripts/
  config/
    generate.mjs               # YAML + modules -> generated; --check для CI
src/
  generated/
    vuespConfig.h              # флаги, дефолты, лимиты
    scriptManifest.h           # события, функции, лимиты VM
    gpioConfig.h               # overrides пинов из hardware.pins
vue/
  src/generated/
    vuespConfig.ts
    modules.ts
    servicePages.ts
    dashboardCards.ts
    pages.json                 # список страниц для Vite exclude
    localeSections.json        # какие секции локалей выключены
    dashboardTypes.json        # разрешённые типы dashboard-карточек
    routes.ts                  # опционально, если уйдём от auto-routes
  json/
    menu.json                  # генерируется из YAML
    # dashboard.json пока остаётся ручным; генерация instances — позже
```

`src/const/protocol.h`, `src/const/protocol.generated.h`, `vue/src/generated/protocol.ts` остаются как есть.

## 6. `vuesp.config.yaml`

### 6.1. Пример

```yaml
# yaml-language-server: $schema=./vuesp.config.schema.json
version: 1

project:
  name: my-device
  board: esp8266
  buildProfile: standard
  firmwareVersion: "5.0.13"
  deviceName: "vuesp."

features:
  script: true
  modbus: false
  serial: false
  discovery: true
  dashboard: true

hardware:
  pins:
    - id: led
      gpio: 13
      mode: output
  buses:
    modbus:
      tx: 17
      rx: 16
      de: -1

network:
  wifi:
    dhcp: true
    ip: 192.168.1.12
    subnet: 255.255.255.0
    gateway: 192.168.1.1
    dns: 8.8.8.8
    ssid: ""
    password: ""
  auth:
    enabled: true
    login: admin
    password: admin
  discovery:
    enabled: true
    interval: 5
    port: 12345

logging:
  level: 2

script:
  cacheSlots: 6
  maxCodeBytes: 512
  stackSize: 64
  scratchBytes: 256
  maxEventParams: 4
  events:
    - id: 1
      name: btn
      args: [gpio, value]
      description: button edge
    - id: 6
      name: time
      args: [hours, minutes]
      description: device uptime clock
  functions:
    - id: 1
      name: fade
      minArgs: 3
      maxArgs: 3
      ret: int
      description: PWM fade

web:
  menu:
    auto: true
  pages:
    gpio: true
    settings: true
    storage: true
  dashboard:
    cards:
      - button
      - dimmer
      - info
      - chart
  locales:
    - ru
    - en
```

### 6.2. Типы секций

| Секция | Назначение |
|---|---|
| `version` | версия формата YAML |
| `project` | имя, плата, профиль, версия прошивки |
| `features` | включённые модули |
| `hardware` | пины и шины |
| `network` | Wi-Fi, auth, discovery |
| `logging` | уровень логов |
| `script` | лимиты, события, функции |
| `web` | меню, страницы, dashboard, локали |

`project.board` валидируется, экспортируется в `VUESP_BOARD` и задаёт
`default_envs` в `platformio.generated.ini`. `project.firmwareVersion` и
`project.deviceName` попадают в generated `[common]` и в `DEF_DEVICE_*`.
Команда `pio run` без `-e` собирает выбранную плату; `pio run -e <env>`
по-прежнему переопределяет выбор.

`features` — основной переключатель модулей. Если модуль выключен, его страницы, меню, локали и компоненты не попадают в сборку, если пользователь явно не переопределил это в `web`.

`web.pages` по умолчанию содержит только базовые страницы (`gpio`, `settings`, `storage`). Страницы модулей наследуются от `features`. При необходимости `web.pages` может переопределить и страницу модуля — это advanced-сценарий.

## 7. Схема модуля

Файл `config/modules/<id>.yaml`:

```yaml
id: modbus
version: 1

label:
  ru: Modbus RTU
  en: Modbus RTU
description:
  ru: Профиль, master/slave и регистры Modbus.
  en: Profile, master/slave and Modbus registers.

category: integration
default: false

requires:
  board: [esp32, pico32, esp32-c3-devkitm-1]
  features: []

conflicts:
  features: []

firmware:
  files:
    - src/modbus
  buildFlags:
    - -DVUESP_MODBUS=1
  defines:
    VUESP_MODBUS: 1
  srcFilter:
    include:
      - +<modbus/*>

web:
  pages: []
  servicePages:
    - id: modbus
      path: /device/modbus/
      component: components/service/ServiceModbus.vue
      menu:
        name: menu.modbus
        icon: modbus
  components:
    - id: modbus-status
      component: components/service/ServiceModbus.vue
      kind: service-block
  locales:
    - modbus

schema:
  - key: bus.tx
    type: gpio
    default: 17
    label:
      ru: TX
      en: TX
    group: bus
  - key: bus.rx
    type: gpio
    default: 16
    label:
      ru: RX
      en: RX
    group: bus
```

### 7.1. Поля модуля

| Поле | Обязательно | Назначение |
|---|---|---|
| `id` | да | стабильный kebab-case id |
| `version` | да | версия дескриптора |
| `label` | да | названия для UI |
| `description` | нет | описание для UI |
| `category` | да | `firmware`, `web`, `integration`, `example` |
| `default` | да | включать по умолчанию |
| `requires` | нет | ограничения по плате и другим модулям |
| `conflicts` | нет | несовместимые модули |
| `firmware` | нет | файлы, флаги, defines, srcFilter |
| `web` | нет | страницы, service pages, компоненты, локали |
| `schema` | нет | поля настроек модуля |

### 7.2. Правила

- `id` не меняется после публикации.
- Модуль не удаляет файлы и не патчит чужие файлы.
- Все пути в `firmware.files` и `web.*.component` проверяются генератором.
- Если модуль выключен, его `firmware` и `web` не применяются.
- `requires.features` и `conflicts.features` ссылаются только на существующие модули.

## 8. Каталог компонентов

Для dashboard и повторяемых блоков используется отдельный каталог `config/components/dashboard.yaml`:

```yaml
id: dashboard-card
items:
  - id: button
    component: vuesp-components/components#CardButton
    label:
      ru: Кнопка
      en: Button
    props:
      - key: icon
        type: icon
        default: socket
  - id: chart
    component: components/dashboard/CardChart.vue
    label:
      ru: График
      en: Chart
```

Пользовательский YAML ссылается на `id`:

```yaml
web:
  dashboard:
    cards:
      - button
      - chart
```

Генератор проверяет, что `id` существует в каталоге, и собирает `dashboardCards.ts`. Сам `dashboard.json` с экземплярами пока остаётся ручным; его генерация — следующий этап.

## 9. Генерация

### 9.1. Пайплайн

```text
vuesp.config.yaml
config/modules/*.yaml
config/components/*.yaml
        │
        ▼
scripts/config/generate.mjs
        │
        ├── src/generated/vuespConfig.h
        ├── src/generated/scriptManifest.h
        ├── src/generated/gpioConfig.h
        ├── vue/src/generated/vuespConfig.ts
        ├── vue/src/generated/modules.ts
        ├── vue/src/generated/servicePages.ts
        ├── vue/src/generated/dashboardCards.ts
        ├── vue/src/generated/pages.json
        ├── vue/src/generated/localeSections.json
        ├── vue/src/generated/dashboardTypes.json
        ├── vue/json/menu.json
        ├── config/modules.generated.json
        ├── config/project.resolved.json
        ├── platformio.generated.ini
        └── vuesp.config.schema.json
```

`vuesp.config.schema.json` строится из схем модулей и компонентов. Он же используется редактором YAML и будущим конструктором.

### 9.2. C++

`src/generated/vuespConfig.h` содержит:

- `VUESP_*` feature flags;
- board id;
- сетевые дефолты;
- уровень логов;
- лимиты VM, если модуль `script` включён;
- стабильные id событий и функций, если они вынесены в YAML.

Правило: generated-заголовок использует `#ifndef`, чтобы `build_flags` могли переопределить значение для CI или отладочной сборки.

`src/const/define.h` и `src/const/features.h` включают generated-заголовок; значения берутся из него, а `build_flags` могут переопределить их.

### 9.3. Web

`vue/src/generated/vuespConfig.ts` — типизированный снимок YAML для UI.

`vue/src/generated/modules.ts` — список включённых модулей и их вклад в меню.

`vue/src/generated/servicePages.ts` — только включённые service-страницы.

`vue/src/generated/dashboardCards.ts` — только выбранные dashboard-карточки.

`vue/src/generated/pages.json` — список отключённых страниц для `unplugin-vue-router`:

```ts
VueRouter({
  routesFolder: 'src/pages',
  exclude: disabledPages.map((page) => `src/pages/${page}/**`),
})
```

`vue/json/menu.json` затем мержится в `public/default.json` существующим `protocol-and-json.js`. Dashboard пока остаётся ручным; его генерация — фаза 2.

### 9.4. Что коммитим

Рекомендация: generated-файлы коммитить, как сейчас `protocol.generated.h` и `protocol.ts`.

Причины:

- свежий клон собирается без обязательного шага генерации;
- diff показывает, что изменилось;
- CI может запускать `npm run config:check`.

Если generated-файлы решено не коммитить, `prebuild` обязан запускать генератор, а CI — `--check` после генерации.

## 10. Композиция web

### 10.1. Страницы и routes

- Включённые страницы попадают в `pages.json`.
- Отключённые страницы исключаются из `unplugin-vue-router`.
- Файлы страниц не удаляются.
- Для полностью управляемых routes предусмотрен будущий переход на generated `routes.ts`; на первом этапе достаточно `exclude`.

### 10.2. Service pages

`device/[page].vue` больше не импортирует все сервисы. Он использует `servicePages.ts`:

```ts
export const SERVICE_PAGES = {
  settings: ServiceSettings,
  gpio: ServiceGPIO,
};
```

Выключенный модуль не попадает в реестр и не попадает в бандл.

### 10.3. Меню

Меню собирается из:

- базовых пунктов (`home`, `settings`, `storage`, `gpio`);
- вкладов включённых модулей (`script`, `modbus`, `serial`);
- `web.menu.extra` из YAML.

Дубли путей и имён — ошибка генерации.

### 10.4. Dashboard

- `web.dashboard.cards` задаёт доступные типы карточек.
- Генератор собирает `dashboardCards.ts` и `dashboardTypes.json`.
- `dashboard.json` с экземплярами остаётся контентным файлом, но
  `protocol-and-json.js` вырезает из него карточки выключенных типов.
- Карточки, которых нет в каталоге, — ошибка.
- Runtime-редактирование dashboard остаётся возможным; YAML задаёт доступный набор.

### 10.5. Локали

Генератор пишет `vue/src/generated/localeSections.json` со списком секций,
которые нужно выключить. `protocol-and-json.js` удаляет их из `default.json`.

Сейчас выключаются:

- `script` и `scriptStudio` при `features.script: false`;
- `serial` при `features.serial: false`;
- `modbus` при `features.modbus: false`.

Базовые секции (`menu`, `homePage`, `common` и т.п.) остаются всегда.

### 10.6. Добавление нового компонента

Если компонента нет в каталоге, YAML не создаёт код. Нужен scaffold:

```text
config/modules/my-widget.yaml
src/modules/my-widget/MyWidget.vue
src/modules/my-widget/index.ts
```

После этого `my-widget: true` в YAML включает компонент. Scaffold — отдельный сценарий конструктора.

## 11. Композиция firmware

### 11.1. Флаги

`vuespConfig.h` задаёт `VUESP_*`-флаги, которые используют существующие `#if` в `main.cpp`, `webserver.cpp`, `gpio.cpp` и других файлах.

### 11.2. Файлы

Для выключенных модулей предусмотрены два режима:

- **Обычная сборка:** файлы остаются, каталог исключается через
  `build_src_filter`, а код закрыт `#if VUESP_*`.
- **Prune/export:** файлы выключенных модулей физически удаляются из
  экспортируемого проекта. Пока не реализовано.

`platformio.generated.ini` содержит `build_src_filter`, который исключает
каталоги выключенных модулей. Сейчас это работает для `script`, `modbus` и
`discovery`: `runner/*`, `modbus/*` и `discovery/*` не компилируются, а
соответствующие include/вызовы закрыты `#if VUESP_*`. `serial` и `dashboard`
не имеют firmware-файлов и управляют только веб-частью.

### 11.3. Скриптовый манифест

Если `script` включён, `src/generated/scriptManifest.h` собирается из
`script.events`, `script.functions` и лимитов.

Если `script` выключен:

- `src/runner` исключается из `build_src_filter`;
- generated `scriptManifest.h` может оставаться на диске, но не компилируется;
- веб-страница, меню и локали скриптов выключаются генератором.

### 11.4. Пины

`hardware.pins` — это overrides поверх штатной таблицы портов платы, а не
полная замена таблицы. Они применяются при старте до чтения `ConfigStore`:
сохранённые на устройстве настройки портов имеют приоритет.

Поля: `id`, `gpio`, `mode`, `value`, `interrupt`, `disabled`, `button`.
Если GPIO нет в таблице платы, override игнорируется.

## 12. Валидация

### 12.1. JSON Schema

`vuesp.config.schema.json` проверяет:

- типы и обязательные поля;
- enum-значения;
- диапазоны;
- неизвестные ключи.

### 12.2. Семантические проверки

- `features` ссылается на известные модули;
- обязательные поля module descriptors (`id`, `label`, `category`);
- зависимости и конфликты модулей;
- совместимость модуля с платой;
- уникальность GPIO и конфликты назначения;
- `hardware.pins` не содержит повторяющихся GPIO;
- `cacheSlots * maxCodeBytes <= 12 КиБ`;
- `maxCodeBytes` ∈ `{256, 512, 1024}`;
- `cacheSlots` ∈ `[1, 15]`;
- уникальные id событий и функций;
- `params <= 4`;
- `minArgs <= maxArgs <= 5`;
- уникальные пути меню и routes;
- существование компонентов и локалей;
- generated-файлы актуальны (`--check`).

### 12.3. Уровни

- **error** — сборка невозможна;
- **warning** — сборка возможна, но есть риск;
- **info** — подсказка конструктору.

## 13. Конструктор

1. Читает `vuesp.config.schema.json`, `config/modules.generated.json` и
   `config/project.resolved.json` (или получает его через `npm run config:preview`).
2. Показывает дерево: плата → модули → страницы → компоненты → пины → лимиты.
3. Валидирует в реальном времени.
4. Показывает preview:
   - какие страницы и меню появятся;
   - какие компоненты попадут в бандл;
   - какие C++-флаги изменятся;
   - какие generated-файлы обновятся;
   - оценка размера и RAM.
5. Пишет `vuesp.config.yaml`.
6. Запускает `npm run config` и сборку.
7. Отдаёт `.bin`, LittleFS или создаёт ветку/PR в текущем шаблоне.

Конструктор не пишет произвольный Vue/C++ код. Он выбирает модули и компоненты из каталога. Для нового кода используется scaffold-сценарий.

## 14. Экспорт и prune

Обычная сборка не удаляет исходники. Для скачивания «обрезанного» проекта:

```bash
npm run config:export -- --prune
```

Prune:

- удаляет файлы выключенных модулей;
- удаляет их routes, меню, локали и компоненты;
- оставляет `vuesp.config.yaml` и generated-файлы;
- формирует standalone-проект для сборки.

Prune — отдельный режим, не часть `build`.

## 15. Миграция

### Фаза 1. Каркас

- `vuesp.config.yaml`;
- `scripts/config/generate.mjs` с режимом `--check`;
- `src/generated/vuespConfig.h`;
- `vue/src/generated/vuespConfig.ts`, `modules.ts`;
- перенос `define.h`, `features.h`, событий, функций и лимитов VM;
- меню и feature flags для web.

### Фаза 2. Web-композиция — реализовано

- `servicePages.ts`;
- `dashboardCards.ts`;
- `pages.json` + `exclude` в Vite;
- generated menu/dashboard;
- фильтрация локалей.

### Фаза 3. Модули прошивки — реализовано, кроме `vuesp-create`

- `config/modules/*.yaml` для `script`, `modbus`, `serial`, `discovery`;
- `srcFilter`/prune;
- `#if VUESP_*` вокруг include и вызовов;
- `platformio.generated.ini` с `build_src_filter`;
- `vuesp-create` начинает читать module descriptors (следующий шаг).

### Фаза 4. Конструктор — schema/catalog/preview готовы, UI и сборка нет

- JSON Schema;
- каталог модулей и компонентов;
- preview;
- сборка и выдача артефактов.

### Фаза 5. Экспорт — не реализовано

- prune;
- standalone ZIP;
- lock-файл версий шаблона.

## 16. Открытые вопросы

1. Генерируем ли `platformio.ini` целиком или только `default_envs`/`[common]`/`build_src_filter`?
2. Коммитим ли generated-файлы или генерируем перед каждой сборкой?
3. Когда переходим от `unplugin-vue-router` + `exclude` к generated `routes.ts`?
4. Нужен ли `vuesp.config.lock` для воспроизводимости версий шаблона и модулей?
5. Когда внешний `vuesp-create` начнёт читать `config/modules/*.yaml`?
6. Делаем ли отдельный export/prune для standalone-проекта?
