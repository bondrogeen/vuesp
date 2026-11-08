# Vuesp — documentation

This file contains the detailed documentation for the project. The catalog on
[vuesp.ru](https://vuesp.ru) renders it separately from the short description
and the README. Replace the examples below with the details of your device.

## Overview

Vuesp is firmware and a local web interface for ESP32, ESP32-C3 and ESP8266.
The device serves the interface itself, so an account, cloud and permanent
internet connection are not required.

## Features

- GPIO inputs and outputs, PWM, inverted signals and safe values;
- Dallas temperature sensors, analog inputs and value history;
- real-time dashboard over WebSocket;
- Lua scripts, periodic tasks and device-side actions;
- LittleFS storage for configuration, dashboard and user files;
- OTA updates over Wi-Fi;
- optional HTTP Basic Authentication;
- light and dark themes based on `vuesp-components`.

## Supported boards

| Board | PlatformIO env | Flash | Filesystem offset |
| --- | --- | --- | --- |
| ESP32 | `esp32` | 4 MB | `0x290000` |
| ESP32-C3 | `esp32-c3-devkitm-1` | 4 MB | `0x290000` |
| ESP8266 | `esp8266` | 4 MB | `0x300000` |

## Web interface

The web UI is built with Vue 3, Vite and Tailwind CSS 4. It uses the shared
[`vuesp-components`](https://github.com/bondrogeen/vuesp-components) library
for the theme, buttons, fields, dialogs and the dashboard.

```bash
cd vue
npm install
cp .env.example .env
npm run dev
```

Set `VITE_IP` in `vue/.env` to the device IP. The development server opens at
<http://localhost:3000> and proxies API requests to the board.

## Firmware

```bash
pio run -e esp32-c3-devkitm-1
pio run -e esp32-c3-devkitm-1 -t upload
```

`npm run build` builds the web UI, updates the generated structs and builds
the firmware for the selected board.

## API

The device exposes:

- HTTP endpoints for the dashboard, files, settings and scripts;
- a binary WebSocket for device state and commands;
- a serial setup protocol (`VUESP:PING`, `VUESP:SCAN`, `VUESP:SET`) used by the
  browser flasher to configure Wi-Fi over USB.

See the [API documentation](https://docs.vuesp.ru/api) for the message layout.

## Filesystem

LittleFS stores:

- `default.json` — device configuration;
- `dashboard.json` — dashboard cards and layout;
- `gpio.json` — port configuration;
- `scripts/` — user scripts;
- `www/` — the built web interface.

## Troubleshooting

- If the board does not appear in the browser, use Chrome, Edge or Opera and a
  data-capable USB cable.
- If flashing fails, hold the BOOT button and retry.
- If the device does not respond after flashing, press RESET and reopen the
  serial port.
- If Wi-Fi setup does not start, check that the firmware includes the serial
  setup protocol.

## Links

- [Project repository](https://github.com/bondrogeen/vuesp)
- [Documentation](https://docs.vuesp.ru)
- [Project catalog](https://vuesp.ru/projects)
- [UI library](https://github.com/bondrogeen/vuesp-components)
