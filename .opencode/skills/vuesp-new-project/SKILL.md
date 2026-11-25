---
name: vuesp-new-project
description: Use when creating a new Vuesp device project, porting a board to the current Vuesp template, adding board-specific firmware/UI/dashboard, or documenting a device project. Covers vuesp-create, PlatformIO, protocol/ConfigStore, dashboard cards, verification and release hygiene.
---

# Vuesp New Project

Скилл для создания и актуализации device-проектов на базе `vuesp`.

## Когда применять

- новый проект `vuesp.<board>`;
- перенос старого board-проекта на актуальный Vuesp;
- добавление платовых пинов, I/O, ADC, RS485/Modbus, Ethernet;
- новые страницы/карточки dashboard и настройки;
- документация и проверка на железе.

## Перед началом

1. Прочитать `../../../CREATING_PROJECTS.md` — полное руководство.
2. Прочитать `../../../vuesp-workspace/docs/code-rules.md`, если workspace доступен.
3. Проверить `git status` и не перезаписывать чужие незакоммиченные изменения.
4. Сначала план, затем изменения. Коммит и пуш — только по явной просьбе.

## Порядок работы

1. **Зафиксировать базу**
   - имя `vuesp.<board>`, версия 5.0.0;
   - `npx vuesp-create@latest` или чистая копия `vuesp`;
   - при миграции старого проекта — backup рабочего дерева и `git diff`.

2. **Плата**
   - `platformio.ini`: только реальные env, board, FS, libs, version;
   - `src/const/define.h`: все пины, PCF/SPI/I2C/UART, Ethernet PHY;
   - `src/gpio/gpio.cpp`: только безопасные пользовательские порты;
   - не смешивать ревизии платы и не обещать отсутствующее железо.

3. **Протокол**
   - `src/const/protocol.h`: `Settings`, `Info`, `Device`, ключи, платовые структуры;
   - версии `CONFIG_*`/`SETTINGS_VERSION`;
   - `npm run protocol` после каждого изменения;
   - обновить дефолты `WebSocketStore`.

4. **Прошивка**
   - `config` — постоянные записи;
   - `eeprom/Settings` — Wi-Fi/auth/сеть;
   - `device` — I/O, ADC, события, ScriptRunner;
   - `init` — Wi-Fi/Ethernet и статус в `Info`;
   - `webserver` — WS/HTTP;
   - атомарные команды для групп (например, RGB), а не три отдельных порта.

5. **Web UI**
   - `menu.json`, локали en/ru, `default.json`;
   - dashboard: входы — info/disabled, выходы — button + `CMD.SET`, ADC — info;
   - для серии устройств предпочитать dashboard-карточки отдельной дублирующей странице;
   - `VCard`/`VButton`/`VIcon`, Tailwind 4 px, `title`/`aria-label`;
   - `vue/json/suggestions.json` — только платовые функции и события; глобальные (`fade`, `udp`, `http`, `btn*`) живут в `vuesp-components/src/components/script/editor/suggestions.ts`;
   - следить за `MAX_EXTERNAL_FUNCTIONS` в `src/runner/ScriptRunner.h`: лишние `registerFunction()` молча не регистрируются;
   - стартовые примеры — в `vue/public/scripts.txt`, после `npm run protocol` и `build-web`;
   - file-зависимость `vuesp-components` требует `npm run build` библиотеки и маппинга `vue`/`@vue/*` в `tsconfig.app.json`.

6. **Проверка**
   - `npm ci`, `npm run protocol`, typecheck, lint, build-web;
   - `pio run`, `pio run -t buildfs`;
   - mock UI и скриншоты;
   - на железе: входы/выходы, ADC, Ethernet DHCP/static, Wi-Fi+Ethernet, RS485/Modbus, OTA, сохранение настроек;
   - фиксировать, что проверено, а что нет.

7. **Документация**
   - `README.md` — кратко;
   - `DOCS.md` — пины, интерфейсы, API, ScriptRunner, ограничения, источники;
   - `.vuesp.json` — метаданные;
   - не добавлять неподтверждённые характеристики.

## Частые ошибки

- `sync` без backup поверх грязного дерева;
- копирование платового кода A6 в A16 без адаптации пинов/PCF/полярности;
- три `KEY.PORT` вместо одной атомарной команды;
- `get`/`set` с hex/битовыми масками вместо поля в `Device`;
- забыть `npm run protocol`;
- `ETH.config` всегда статический при наличии DHCP;
- `data/www`, `build`, `node_modules` в git;
- отдельная страница I/O вместо dashboard-карточек серии;
- дублирование глобальных suggestions (`fade`, `udp`, `http`) в проекте;
- примеры с устаревшими `$v0` вместо `$u0`;
- `boot`-скрипт, который сразу включает реле или выходы;
- больше `registerFunction()`, чем `MAX_EXTERNAL_FUNCTIONS`, — часть функций пропадает без явной ошибки.

## Результат

В ответе указать:

- что изменено;
- какие env/размеры прошивки;
- что проверено на железе;
- что осталось непроверенным;
- что не коммитилось.
