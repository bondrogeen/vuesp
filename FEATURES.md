# Features and module configuration

The template ships the full feature set. Features are enabled or disabled in
[`vuesp.config.yaml`](./vuesp.config.yaml); the generator derives firmware build
filters, web pages, menu, locales and protocol-facing config from it.

The legacy machine-readable list in [`features.json`](./features.json) is kept
for `vuesp-create` compatibility. New module descriptors live in
[`config/modules/*.yaml`](./config/modules); their generated catalog is
`config/modules.generated.json`.

## Base

Always present:

```text
firmware  main, config/ConfigStore, const, device, diag, eeprom, files,
          gpio, init, libs, setup, tasks, utils, webserver
web       app shell, router, stores, composables, base pages
          (home, device settings/storage/gpio)
```

## Modules

| Module | Firmware | Web | Default |
| --- | --- | --- | --- |
| `script` | `src/runner` | `/script/` studio, compiler, script composables | on |
| `serial` | — | `/serial/` example console | on |
| `modbus` | `src/modbus` | Modbus service page, register map, frame helpers | off |
| `discovery` | `src/discovery` | Discovery settings card | on |
| `dashboard` | — | `/dashboard/` cards and editor | on |

Disable a module in YAML:

```yaml
features:
  modbus: false
  serial: false
  discovery: false
  dashboard: false
```

Then regenerate:

```bash
cd vue
npm run config
npm run build-web
cd ..
pio run
```

What happens:

- disabled firmware modules are excluded from `build_src_filter`;
- guarded includes/calls are compiled out with `#if VUESP_*`;
- disabled top-level pages are excluded from the router;
- disabled service pages are removed from `servicePages.ts`;
- disabled menu items and locale sections disappear;
- disabled dashboard card types are filtered out of `default.json`.

Source files are **not deleted**. The module simply leaves the build and the web
composition. A separate export/prune mode can produce a physically trimmed
project later.

## Enabling Modbus

Modbus is off by default because the profile and receive buffers take about
21 KB of static RAM, which is too much for ESP8266.

```yaml
project:
  board: esp32

features:
  modbus: true
```

Then:

```bash
cd vue
npm run config
cd ..
pio run
```

Rules:

- Modbus is allowed only on `esp32`, `pico32` and `esp32-c3-devkitm-1`;
- `features.modbus: true` on `esp8266` is a generation error;
- the generated `build_src_filter` includes `src/modbus` only when enabled;
- TX/RX/DE pins remain runtime settings in the UI; `DE = -1` means auto-direction.

## Pins

Use `hardware.pins` for overrides of the board port table:

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

Overrides are applied at boot before `ConfigStore` is read. Saved port settings
on the device win. If a GPIO is not present in the board table, the override is
ignored.

## Legacy `vuesp-create`

`features.json` still lists files for `vuesp-create` removal and sync. It is not
the source of truth for the current template build: YAML and module descriptors
are. The external `vuesp-create` integration with `config/modules/*.yaml` is a
planned next step.

## Known issues

- `vuesp-create` does not read `config/modules/*.yaml` yet;
- `platformio.ini` is a shell; generated values live in `platformio.generated.ini`;
- `dashboard.json` instances remain content; YAML controls available card types;
- `hardware.pins` is an override list, not a full board pin table.
