# Design

## Context

See `proposal.md` — Why. The current text interpreter keeps up to 256 source
characters per slot, re-parses one token per 10 ms, and occupies about 7.4 KB
of static RAM. The target is ESP8266 (80 KB RAM, 4 MB flash) and ESP32. The
firmware is cooperative: `loop()` calls `loopRunner(now)`, `wait()` must not
block, and HTTP/WebSocket work is served by `ESPAsyncWebServer`.

The new engine is additive. `src/runner/ScriptRunner.*` stays untouched and
keeps serving `/cmd` and `scripts.txt` until the new path is enabled per board.

## Goals / Non-Goals

**Goals:**

- Move all parsing off the MCU: the device only verifies and executes bytecode.
- Preserve every current language feature and provider contract.
- Standard expression precedence, compile-time diagnostics with positions, and
  a lossless source ↔ bytecode round-trip.
- At least 10× more script work per loop iteration and RAM not worse than the
  current engine.
- Deterministic compilation for the same source and capability profile.
- Keep the change reviewable and testable in isolated phases.

**Non-Goals:**

- Full JavaScript, closures, dynamic typing beyond current slots, or recursion.
- Replacing the text engine or migrating `scripts.txt`.
- Hand-editing bytecode in the UI; hex and disassembly are read-only.
- Cross-device bytecode portability: bytecode is compiled for a capability
  profile (function/event ids) and is not portable between different profiles.
- User-defined functions in v1; they are a v1.5 extension.

## Decisions

### 1. Stack VM with structured metadata

Use a stack-based VM with tagged values. The compiler emits jumps for control
flow and also writes a block table (`IF`, `ELSE`, `WHILE`, `HANDLER`,
`TERNARY`) and a handler table. The VM ignores the block table; the verifier
and decompiler use it to reconstruct control flow exactly.

Alternatives considered:

- **Token IR** — less work, but still dispatches token kinds and does not give
  real verification or a compact round-trip. Rejected as the final format.
- **Register VM** — faster for numeric code, but a larger compiler, no clear
  win for small scripts, and harder decompilation. Rejected for v1.
- **AST/CBOR** — the MCU would still walk a tree; usually larger and slower
  than bytecode. Rejected.

### 2. Value model and memory budget

A value is an 8-byte tagged union: `type` plus int32/uint32/float/pointer+len.
The VM uses one shared value stack of 64 entries (512 B). Variables remain
global fixed slots, as today: `$u0..$u9`, `$i0..$i9`, `$f0..$f4`, `$s0..$s1`,
`$a0..$a4`, `$p0..`, `$e0..$e3`, `$c0..$c3`. `$name` is resolved through the
existing `DataProvider`; `$pN` through `PortProvider`.

String and array storage stays in the shared global context. Expression
temporaries use a small scratch arena (4 × 64 B) reset at statement boundaries.
Per-context state is about 16 B; 15 contexts cost about 240 B. Code is loaded
into a bounded RAM cache (default 3 slots × 1 KB, `VBC_CACHE_SLOTS`, range
1..15) from flash on run/event; inactive entries are evicted.

### 3. Instruction encoding

- 1-byte opcode, little-endian operands, variable length.
- Variable operand: one byte `(type << 4) | index`.
- Constant operand: `u16` index into the typed pool.
- Jump target: absolute `u16` offset in the code section.
- External call: `u8 function id`, `u8 argc`; event: `u8 event id`.
- `WAIT` stores milliseconds as `u32`; the compiler normalizes `ms/s/m/h`.

`AND` and `OR` are non-short-circuit in v1 so the decompiler reconstructs them
unambiguously; this matches the current left-to-right evaluator. Short-circuit
semantics can be added in v1.5 with a dedicated block kind.

### 4. Container layout

```
magic "VBC1" | version | flags | header_size
code_len | const_count | block_count | handler_count
entry_offset | stack_max | source_hash | code_crc32
const pool | code | block table | handler table | optional source map
```

The constant pool is typed (`INT32`, `UINT32`, `FLOAT32`, `STRING`, `NAME`,
`ARRAY`). The handler table maps event id → entry/end. The source map maps code
offsets to line/column and is optional (flash budget); the compiler includes it
by default for scripts up to the configured limit.

### 5. Compiler and decompiler

One TypeScript module under `vue/src/lib/script/` is shared by the web UI and
tests:

```
lexer → parser → AST → semantic checks → codegen → serializer
                                                 ↓
                                          .vbc / hex / disasm
                                                 ↑
decompiler ← block table + stack simulation ← reader
```

The codegen is canonical: no optimization passes in v1, fixed statement order,
fixed formatting. The decompiler walks instructions, rebuilds expressions by
simulating the value stack, and uses the block table for `if/else`, `while`,
`on` and ternary. This makes `compile(decompile(x))` equal to `x` and
`decompile(compile(s))` stable.

Semantic checks cover block balance, slot bounds, arity, known functions and
events, string/array limits, stack depth and code size. Errors carry a stable
code plus `line:column`.

Tests use Node 24's built-in TypeScript stripping and `node --test`, so no new
test runner dependency is required.

### 6. Capability manifest and stable ids

The device exposes a JSON manifest: VM version, limits, slot counts, external
functions (`id`, `name`, `min`, `max`, `ret`) and events (`id`, `name`,
`params`). Firmware registers external functions with explicit numeric ids;
the legacy text engine keeps its name-based `registerFunction`.

The compiler fetches the manifest and resolves names to ids. Unknown functions
or events are compile errors. This removes the current drift between
`suggestions.json` and the functions actually registered on a board.

### 7. Storage and transport

- Flash: `/www/scripts.bc`, a header plus TOC plus code blobs. Writes use
  `scripts.bc.tmp` → flush → rename; a failed write leaves the old file intact.
- Flash is the source of truth; RAM holds the TOC, a `name → id` map and the
  active code cache (default 3 slots).
- HTTP: `GET /script/list`, `GET/POST/DELETE /script/bytecode?name=N`,
  `POST /script/run?name=N`, `GET /script/capabilities`, `GET /script/status`.
  Uploads and downloads are binary and bypass `Message.text[256]`.
- Run/stop/status can also use existing WebSocket messages. Existing `/cmd`,
  `/fs`, `/update` and `/esp` are untouched.
- The web page fetches list + bytecode, decompiles on open, compiles on save,
  shows diagnostics/hex/disassembly, and uploads. It can download a `.vbc`
  file for inspection.

### 8. Execution model

`update()` visits contexts round-robin. Each context gets at most
`INSTR_BUDGET` (default 32) or `TIME_BUDGET_US` (default 300 µs), then yields.
`update()` has a global budget (default 3 ms). `WAIT` stores `wait_until` and
suspends the context; the minimum wait remains 10 ms.

Events use a free context from the same pool. In v1, if none is free, the event
is logged and dropped; v1.5 adds a small queue. Re-registering a script
replaces its handlers; duplicate handlers for one event are a compile error.

### 9. Errors and diagnostics

- Compile errors: stable code, `line:column`, human-readable message.
- Verifier errors: stable code, section/offset, rejected before execution.
- Runtime errors: script id, instruction offset, source line when available,
  stable code; the context halts and other contexts continue.
- Errors are sent to the web through the existing log/notification path plus a
  status endpoint; they are not silent log lines.

### 10. Single source of truth for opcodes

An `opcodes.yaml`/JSON definition generates the C++ enum/tables and the TS
constants/disassembler metadata. CI checks that generated files are current.
This prevents format and VM drift.

## Risks / Trade-offs

- **Decompiler fidelity vs optimization** → canonical codegen and block tables
  in v1; optimizations only if round-trip tests stay exact.
- **ESP8266 RAM/flash** → early host and on-device measurements, bounded code
  cache, configurable limits, source map optional.
- **LittleFS read latency** → load code into RAM on run/event; cache active
  scripts; eviction only for inactive entries.
- **Format/VM drift** → generated opcode tables and CI check.
- **Handler context exhaustion** → v1 logs and drops; v1.5 adds a bounded
  event queue.
- **No local C++ compiler** → C++ tests run in CI (g++/clang++); TypeScript
  tests and typecheck run locally.
- **Security** → verifier rejects malformed containers; bytecode contains no
  raw pointers, only validated ids/indices; size limits protect flash/RAM.
- **Scope creep** → v1 is parity plus diagnostics; user functions and
  short-circuit logic are explicitly v1.5.

## Migration Plan

1. **Phase 0** — this OpenSpec change, opcode spec, format spec.
2. **Phase 1** — TypeScript compiler/decompiler, serializer, disassembler,
   golden vectors and round-trip tests.
3. **Phase 2** — C++ reader, verifier, VM and host tests against the same
   vectors; no firmware integration yet.
4. **Phase 3** — `scripts.bc` store, `/script/*` endpoints, capability
   manifest, status/errors.
5. **Phase 4** — web studio: editor, diagnostics, hex/disasm, upload/run,
   decompile-on-open.
6. **Phase 5** — enable behind `ENABLE_BYTECODE_VM`, on-device smoke tests,
   document the new language and limits. The old engine remains the default.

Rollback: disable the flag and keep using `ScriptRunner`; no data migration is
required because `scripts.txt` is untouched.

## Open Questions

- Exact v1.5 priorities for user-defined functions, event queue, short-circuit
  logic and dynamic port indices — can be decided after v1 measurements.
- Whether source maps stay enabled on ESP8266 release builds — default on until
  flash measurements say otherwise.
- Which additional boards join the first test matrix beyond ESP8266
  (`wemos-d1-mini`) and ESP32 (`kc868-a16`).
