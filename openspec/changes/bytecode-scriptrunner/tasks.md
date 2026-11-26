# Tasks

## 1. Specification and test stand

- [x] 1.1 Create `feature/bytecode-scriptrunner` and the OpenSpec change; verify `openspec validate bytecode-scriptrunner --strict` passes.
- [x] 1.2 Verify `/dev/ttyUSB0`, serial boot logs, `VUESP:PING` and HTTP `/cmd` on the ESP8266; verify the device returns `@VUESP:PONG` and `{"state":true}`.
- [x] 1.3 Write proposal, spec and design; verify `openspec validate bytecode-scriptrunner --strict` and `openspec status --change bytecode-scriptrunner`.
- [x] 1.4 Write `docs/scriptrunner/bytecode-v1.md` and `opcodes.yaml`; verify the documented container and opcode table match the generated C++/TS constants.

## 2. TypeScript compiler and decompiler

- [x] 2.1 Add lexer, parser and AST for the JS-like syntax with fixed slots; verify parser unit tests for expressions, blocks, handlers and comments.
- [x] 2.2 Add semantic checks for blocks, slot bounds, arity, known functions/events and code/stack limits; verify each error has a stable code and `line:column`.
- [x] 2.3 Add codegen, container serializer and CRC; verify deterministic byte-identical output and golden bytecode vectors.
- [x] 2.4 Add disassembler and decompiler with canonical printing; verify compile→decompile→compile and decompile→compile→decompile round-trips.
- [x] 2.5 Add capability-manifest client and profile-aware compilation; verify unknown functions/events are rejected and ids match the manifest.
- [x] 2.6 Add `npm run test:script` using Node's TypeScript support; verify all compiler tests pass with `node --test`.

## 3. C++ format, verifier and VM

- [x] 3.1 Add container reader and verifier (magic, version, CRC, bounds, jumps, stack, ids); verify unit tests and a 10k-input fuzz run that never crashes.
- [x] 3.2 Add VM core for values, slots, expressions, `if/else`, `while`, ternary and `wait`; verify execution against the TS golden vectors.
- [x] 3.3 Add providers, commands, events, `call` and external functions; verify parity scenarios from the spec.
- [x] 3.4 Add per-context and global execution budgets, round-robin, wait resume and runtime error reporting; verify host tests for yielding, fairness and error isolation.
- [x] 3.5 Add a bounded code cache and measure RAM/flash on ESP8266; verify the target build stays within the agreed limits.

## 4. Firmware storage and transport

- [x] 4.1 Add `/www/scripts.bc` store with TOC and atomic temp→rename writes; verify store/reload, failed-write preservation and untouched `scripts.txt`.
- [x] 4.2 Add `/script/list`, `/script/bytecode`, `/script/run`, `/script/capabilities` and `/script/status`; verify with authenticated curl on the device.
- [x] 4.3 Wire runtime errors and dropped events to status/log output; verify a failing script stops only its context and reports id/offset/line.
- [x] 4.4 Add `ENABLE_BYTECODE_VM` and board configuration; verify the default build keeps the text engine and the flag-enabled build uses bytecode for bytecode ids.

## 5. Web studio

- [x] 5.1 Add the script studio tab with editor, compile diagnostics, hex and disassembly; verify `npm run typecheck`, `npm run lint` and a browser smoke test.
- [x] 5.2 Add decompile-on-open, save/upload/run and `.vbc` download; verify the full round-trip against the device.
- [x] 5.3 Add capability-aware suggestions from `/script/capabilities`; verify unknown symbols are not suggested and known symbols compile.

## 6. Integration and release

- [x] 6.1 Run an end-to-end ESP8266 scenario: upload, run, `wait`, event handler, external function, decompile; verify serial logs and HTTP responses.
- [ ] 6.2 Run the same scenario on an ESP32 board (kc868-a16) when available; verify the same behavior and memory limits.
- [ ] 6.3 Update `github/docs/scriptrunner/` and `vuesp-workspace/docs/decisions.md` if contracts change; verify documented commands run as written.
- [ ] 6.4 Add CI jobs for C++ bytecode tests and TS compiler tests; verify GitHub Actions is green.
- [ ] 6.5 Open a PR to the feature branch, do not merge; verify all PR checks pass.

## Workflow follow-up

- Archive the change after review and merge.
- Enable the flag per board only after the on-device checks pass.
