# ScriptRunner — bytecode scripts

Version: 6.0.0
Updated: 2026-11-28

> The only engine: the source is compiled in the browser into a VBC1 container,
> the firmware verifies it and executes it. The text runner and `scripts.txt`
> are removed. Format details: [bytecode-v1.md](./bytecode-v1.md).

## Introduction

ScriptRunner controls ports, variables, events and external device functions.
Scripts are written in the `/script/` studio, compiled in the browser and stored
on the device as a single `/www/scripts.bc` pack.

Highlights:

- compilation and diagnostics run in the browser; the MC executes verified bytecode;
- up to 15 scripts; the script named `boot` starts automatically;
- `wait()` does not block `loop()`;
- an error stops only its own script and is reported by `KEY_SCRIPTSTATUS` over WebSocket;
- the studio can import a source file (`.js`, `.txt`, `.vjs`) and export the current script as `.js`;
- status is shown by the tab dot, logs open in a separate panel.

## Basic Rules

- The syntax is JavaScript-like, but variables are fixed slots.
- Statements are separated by `;`; line breaks are allowed.
- Comments: `//` and `/* */`.
- Strings use single or double quotes with escapes.
- A script without `on(...)` runs top to bottom; handlers are triggered by events.

## Variables

| Slot | Meaning | Available |
|---|---|---|
| `$uN` | unsigned integer | 0–9 |
| `$iN` | signed integer | 0–9 |
| `$fN` | float | 0–4 |
| `$sN` | string (up to 31 chars) | 0–1 |
| `$aN` | array (up to 64 bytes) | 0–4 |
| `$pN` | GPIO (pin number) | 0–39 |
| `$eN` | event parameters | 0–3, read-only |
| `$cN` | `call()` parameters | 0–3, read-only |

`$u/$i/$f/$s/$a` are local to the loaded script: the same name in different
scripts means different variables. `$p` is shared hardware. Local variable
memory is limited by `script.varPoolBytes` (default 3072 B, minimum 2048 B).

`$name` (for example `$display`) is a device data variable. Available names and
types come from `/vm/capabilities` and the device suggestions.

## Working with Ports

```js
$p13 = 255;      // write (0–255 for PWM)
$u0 = $p13;      // read
```

The number is the GPIO. Input/output mode comes from the device port settings;
writing an input does not change its mode.

## Events

```js
on('btn_c', (gpio, value, count) => {
  log('gpio', gpio, 'value', value, 'count', count);
});
```

Events: `btn`, `btn_c`, `btn_l`, `btn_r`, `time`; `modbus` is available when
enabled. Prefer named handler parameters:
`on('btn_c', (gpio, value, count) => { ... })`. They are local to that handler
and read-only. The low-level `on('btn_c') { ... $e0 ... }` form is still
supported. `time` passes `hours, minutes` from the device uptime clock.

The event and function list is generated from `vuesp.config.yaml` into `src/generated/scriptManifest.h`;
`/vm/capabilities` is authoritative for a running device.

## Control Structures

```js
if ($u0 > 10) {
  $p13 = 255;
} else {
  $p13 = 0;
}

while ($u0 < 5) {
  $u0 = $u0 + 1;
  if ($u0 == 3) { continue; }
  if ($u0 == 5) { break; }
  wait(100ms);
}
```

`wait(10ms | 1s)` pauses only the current script. `return` ends an event handler.

## Logging (log)

```js
log('text', $u0, $s0);
```

Up to 8 arguments. Messages appear in the studio log panel and in the
`KEY.MESSAGE` WebSocket message.

## Expressions and Operators

- arithmetic: `+ - * / %`, unary `-`;
- bitwise: `& | ^ ~ << >>`;
- comparisons: `== != < <= > >=`;
- logical: `&& || !`;
- ternary: `$u0 > 5 ? 100 : 0`;
- parentheses and standard precedence.

> `&&`/`||` do not short-circuit — both sides are always evaluated.

## Working with Arrays

```js
$a0 = [1, 2, 3];
$u0 = get($a0, 1);
set($a0, 1, 9);
$u1 = len($a0);
```

Arrays hold bytes 0–255, up to 64 items. Indexing: `$a0[1]`.

## Working with Strings

```js
$s0 = 'hello';
$u0 = len($s0);
$u1 = ord($s0);
$s1 = chr(65);
```

Strings are concatenated with `+`. A `$sN` variable holds up to 31 characters.

## External Functions and Data

- `fade(gpio, value, time)` — smoothly change PWM/DAC;
- `udp(id, text)` — send text to a device discovered by Vuesp Discovery;
- `http(method, url, ...)` — HTTP request;
- `var(id [, value])` — Modbus profile variable (when Modbus is enabled);
- `modbus(device, point [, value])` — Modbus profile point (when Modbus is enabled).

The authoritative function/event list is `/vm/capabilities`; hint descriptions
live in the device project's `vue/json/suggestions.json`.

## Example Scripts

```js
// boot
log('boot');
$p13 = 128;
wait(500ms);
$p13 = 0;
```

```js
// button
on('btn_c', (gpio, value, count) => {
  log('click', gpio, value, count);
  $p13 = value ? 255 : 0;
});
```

```js
// chain
call('display');
```

`call('name', ...)` starts another saved script; arguments are available as
`$c0…$c3`. There is no return value and recursion is forbidden.

## Limitations

- up to 15 saved scripts, 6 RAM cache slots by default (`VBC_CACHE_SLOTS`);
- up to 512 bytes per script container by default (`VBC_MAX_CODE_BYTES`; profiles 256/512/1024);
- stack up to 64 values;
- up to 4 `call()` arguments, call depth 4;
- 32 instructions or 300 µs per script per pass, 3 ms total budget;
- script names: lowercase, 1–16 characters.

## Quick Reference

```js
$u0 = 0;                    // variable
$p13 = 255;                 // GPIO
$s0 = 'ok';
log($s0, $u0);
if ($u0 > 0) { ... } else { ... }
while ($u0 < 5) { $u0 = $u0 + 1; wait(100ms); }
on('btn_c', (gpio, value, count) => { log(gpio, value, count); });
call('other');
```
