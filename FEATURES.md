# Features and removal guide

The template ships the full feature set. New projects can remove what they do not
need. The machine-readable list lives in [`features.json`](./features.json); this
document explains the manual steps and the planned `vuesp-create` integration.

## Base

Always present (GPIO is part of the base):

```text
firmware  main, config/ConfigStore, const, device, diag, discovery, eeprom,
          files, gpio, init, libs, setup, tasks, utils, webserver
web       app shell, router, stores, composables, base pages
          (home, dashboard, device settings/storage/gpio), dashboard.json
```

## Features

| Feature | Firmware | Web | Menu |
| --- | --- | --- | --- |
| `script` — ScriptRunner | `src/runner` | script page, script composables, script assets | `menu.script` |
| `serial` — SERIAL example | — | `vue/src/pages/serial` | `menu.serial` |
| `modbus` — Modbus RTU example | `src/modbus` | Modbus page, register map, frame helpers | `menu.modbus` |

Protocol keys stay in `protocol.h` even when a feature is removed: they are small
and keep the generated web protocol compatible.

## Removing a feature manually

### script (ScriptRunner)

1. Delete `src/runner`, `vue/src/pages/script`, `vue/src/composables/script`,
   `vue/src/assets/js/script`.
2. Remove `menu.script` from `vue/json/menu.json`.
3. In `src/main.cpp` remove the runner include and the `setupRunner()` /
   `loopRunner(now)` calls.
4. In `src/webserver/webserver.cpp` remove the `scriptRunner.*` calls.
5. Locale keys may stay; they are unused and tiny.

### serial (SERIAL example)

1. Delete `vue/src/pages/serial`.
2. Remove `menu.serial` from `vue/json/menu.json`.
3. Remove `serial.*` from `vue/json/locales/ru.json` and `en.json`.
4. No firmware changes: the page uses a demo transport.

### modbus (example)

1. Delete `src/modbus`.
2. Delete `vue/src/components/service/ServiceModbus.vue`, `ModbusMapDetails.vue`,
   `modbusMapTypes.ts` and `vue/src/utils/modbusFrame.ts`.
3. Remove `menu.modbus` from `vue/json/menu.json`.
4. Remove `modbus.*` and `menu.modbus` from `vue/json/locales/ru.json` and `en.json`.
5. Nothing else references the module; keep the protocol keys.

The example is **off by default**: `VUESP_MODBUS` is `0`, so `src/modbus` stays
in the repository but is not compiled into the firmware. This keeps the default
build lean: the Modbus profile and receive buffers take about 21 KB of static
RAM, which is too much for ESP8266 and makes the web UI fail with OOM.

To enable it on ESP32, add the flag to that environment:

```ini
build_flags = -D VUESP_MODBUS=1
```

Then return `menu.modbus` to `vue/json/menu.json` and run `npm run protocol`
(or add the item back to `vue/public/default.json`). The user selects
Master/Slave, sets TX/RX/DE pins and presses Connect. Pins are runtime settings;
`DE = -1` means auto-direction. The template map is generic:
firmware version, uptime, address, baud, role, last error, requests, errors,
RTC, plus the dynamic slave bank `100–131`.

## Planned: `vuesp-create --without`

Default generation keeps every feature. The user removes what is not needed:

```bash
vuesp-create my-device --board esp32 --without serial,script
```

`vuesp-create` reads `features.json`, deletes the listed files and menu entries,
and records the choice in `.vuesp.json` (`features`), so `sync` can re-apply the
removal after a template update.

## Known issues

- `src/modbus` is compiled only with `-D VUESP_MODBUS=1`; the default build
  leaves it out. Enabling it on ESP8266 is blocked at compile time because the
  web UI runs out of heap (`-D VUESP_MODBUS_ALLOW_ESP8266=1` overrides that).
