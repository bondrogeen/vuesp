# Proposal

## Why

The current ScriptRunner keeps source text in RAM and re-parses one token per
10 ms on the MCU. This caps script speed, wastes RAM, hides syntax and arity
errors until runtime, and prevents the editor from validating code before
upload. Splitting the work — the web compiles and decompiles, the MCU executes
verified bytecode — removes parsing from the hot path and enables compile-time
diagnostics plus a lossless source ↔ bytecode round-trip.

## What Changes

- Add a new, independent `BytecodeRunner` for ESP8266/ESP32. The existing
  text `ScriptRunner` stays untouched.
- Add a versioned bytecode container (`.vbc`) with code, constants, block and
  handler tables, optional source map, CRC and a load-time verifier.
- Add a stack VM with tagged values, structured control flow, `WAIT`,
  script/external calls, events and the existing providers.
- Add a TypeScript compiler and decompiler for a JavaScript-like language with
  fixed slots (`$uN`, `$iN`, `$fN`, `$sN`, `$aN`, `$pN`, `$eN`, `$cN`).
- Add a capability manifest so the web compiles against the functions and
  events actually registered on the device.
- Add HTTP endpoints for list/download/upload/delete/run and a flash store
  (`scripts.bc`). `Message.text[256]` remains for the legacy path.
- Add a web studio with editor, compile diagnostics, hex/disassembly,
  upload/run and decompile-on-open.
- **BREAKING (new engine only):** the new engine reads `.vbc`, not
  `scripts.txt`. No backward compatibility is required; the old engine remains
  available behind a build flag.

## Capabilities

### New Capabilities

- `bytecode-scriptrunner`: bytecode format, verifier, VM execution, storage,
  HTTP transport, web compiler/decompiler and round-trip guarantees.

### Modified Capabilities

None. Existing `firmware-runtime` and `web-interface` requirements are
unchanged because the new engine is additive and disabled by default.

## Impact

- New code: `src/runner/bytecode/`, `src/tests/test_bytecode.cpp`,
  `vue/src/lib/script/`, a new script page/tab.
- New HTTP routes under `/script/*`; existing `/cmd`, `/fs`, `/update` and
  `/esp` keep their behaviour.
- New flash file `/www/scripts.bc`; existing `scripts.txt` is untouched.
- CI: new C++ VM tests and TypeScript compiler tests.
- Rollout: `ENABLE_BYTECODE_VM=0` by default; the new engine is tested
  separately before being enabled per board.
