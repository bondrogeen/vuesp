# Spec Delta

## Purpose

Defines the bytecode-based ScriptRunner: the device stores and executes
verified bytecode, while the web compiles a JavaScript-like source to bytecode
and decompiles it back without losing behavior.

## ADDED Requirements

### Requirement: Bytecode container contract

The device SHALL accept a versioned bytecode container identified by magic
`VBC1`, a version byte and a CRC. It SHALL contain code, a typed constant pool,
a block table, a handler table and an optional source map. The web SHALL
produce and consume the same container.

#### Scenario: Valid container is accepted

- **WHEN** the web uploads a container with a supported version and a valid CRC
- **THEN** the device stores it and reports success

#### Scenario: Unsupported version is rejected

- **WHEN** the container version is not supported by the device
- **THEN** the device rejects it with a version error and stores nothing

#### Scenario: Corrupted container is rejected

- **WHEN** the CRC or a section size does not match the payload
- **THEN** the device rejects it with a corruption error before execution

### Requirement: Pre-execution verification

Before executing bytecode, the device SHALL verify jump targets, block and
handler offsets, constant indices, variable ids, stack depth, external function
ids and event ids. Invalid bytecode MUST be rejected without execution and
without crashing the firmware.

#### Scenario: Out-of-range jump is rejected

- **WHEN** a jump target points outside the code section or into the middle of
  an instruction
- **THEN** the verifier rejects the container

#### Scenario: Stack underflow is rejected

- **WHEN** an instruction consumes more values than the stack contains
- **THEN** the verifier rejects the container

#### Scenario: Unknown external function is rejected

- **WHEN** the code references a function id absent from the capability manifest
- **THEN** the verifier rejects the container

### Requirement: Language feature parity

The bytecode engine SHALL support the current language features: fixed slots
`$uN`, `$iN`, `$fN`, `$sN`, `$aN`, `$pN`, `$eN`, `$cN`; `$name` through the
data provider; arithmetic, bitwise, comparison, logical and ternary
expressions; parentheses; `if/else`; `while`; `wait`; `log`; `call`; `set`,
`get`, `len`, `chr`, `ord`; external functions with parameters; event handlers;
and `return`.

#### Scenario: Port write and read

- **WHEN** a script assigns to `$pN` and later reads it
- **THEN** the port provider receives the write and returns the stored value

#### Scenario: Data provider write

- **WHEN** a script assigns a string to `$display`
- **THEN** the data provider receives the value with the correct kind

#### Scenario: External function with parameters

- **WHEN** a script calls a registered external function with arguments
- **THEN** the function receives the arguments and its return value is available
  to the script

#### Scenario: Event parameters

- **WHEN** an event with parameters fires and a handler is registered
- **THEN** the handler observes the parameters in `$e0…$e3`

#### Scenario: Call by name

- **WHEN** a script executes `call('blink', 1, 2)`
- **THEN** `blink` is loaded from `/www/scripts.bc` on demand and observes
  `$c0 == 1`, `$c1 == 2`; the caller does not wait for a return value

#### Scenario: Recursive call is rejected

- **WHEN** `a` calls `b` and `b` calls `a`
- **THEN** the VM reports a recursion error and stops only the offending context

#### Scenario: Non-blocking wait

- **WHEN** a script executes `wait(100ms)`
- **THEN** the context suspends and the main loop continues serving other work

### Requirement: Compile-time diagnostics

The web compiler SHALL reject invalid source with a stable error code and a
line/column position. It SHALL detect syntax errors, unbalanced blocks,
out-of-range slots, wrong arity, unknown functions or events, and code/stack
limit violations. For the same source and capability profile, compilation SHALL
be deterministic and byte-identical.

#### Scenario: Unknown function

- **WHEN** the source calls a function absent from the manifest
- **THEN** compilation fails with an unknown-function error and its position

#### Scenario: Wrong arity

- **WHEN** the source calls a function with fewer or more arguments than allowed
- **THEN** compilation fails with an arity error and its position

#### Scenario: Unbalanced block

- **WHEN** an `if`, `while` or `on` block is not closed
- **THEN** compilation fails with a block error and its position

#### Scenario: Deterministic compile

- **WHEN** the same source and capability profile are compiled twice
- **THEN** the two containers are byte-identical

### Requirement: Source ↔ bytecode round-trip

For every container produced by the compiler, decompiling it SHALL produce
canonical source that compiles back to a semantically equivalent container.
Control flow, expressions, handlers and constants SHALL be preserved.

#### Scenario: Round-trip control flow

- **WHEN** source with `if/else` and `while` is compiled and decompiled
- **THEN** compiling the decompiled source preserves the same branches and loop

#### Scenario: Round-trip expressions

- **WHEN** source with a ternary, parentheses and string concatenation is
  compiled and decompiled
- **THEN** the decompiled source preserves the expression tree

#### Scenario: Round-trip event handler

- **WHEN** source with `on('btn')` is compiled and decompiled
- **THEN** the handler and its event id are preserved

### Requirement: Capability manifest

The device SHALL expose a capability manifest containing the VM version,
limits, variable slot counts, external function ids, names, arity and return
type, and event ids, names and parameter counts. The web compiler SHALL compile
against the manifest and reject unknown symbols.

#### Scenario: Manifest fetch

- **WHEN** the web requests the capability manifest
- **THEN** the device returns the current VM version, limits, functions and
  events

#### Scenario: Compile against device profile

- **WHEN** the web compiles source for a device
- **THEN** it uses that device's function and event ids

#### Scenario: Unknown event is rejected

- **WHEN** the source registers a handler for an event absent from the manifest
- **THEN** compilation fails with an unknown-event error

### Requirement: Flash storage and atomic update

The device SHALL persist bytecode in `/www/scripts.bc` as a table of contents
plus code blobs. Uploads SHALL be atomic: a failed write MUST leave the previous
content intact. The existing `/www/scripts.txt` SHALL remain untouched.

#### Scenario: Store and reload

- **WHEN** a container is uploaded and the device restarts
- **THEN** the container is still available for download and execution

#### Scenario: Failed write keeps previous content

- **WHEN** an upload is interrupted or fails verification
- **THEN** the previous `/www/scripts.bc` content remains readable

#### Scenario: Legacy file unchanged

- **WHEN** the bytecode store is used
- **THEN** `/www/scripts.txt` is not modified

### Requirement: HTTP bytecode transport

The device SHALL provide authenticated endpoints to list, download, upload,
delete and run scripts by name. Upload and download SHALL use binary payloads
and MUST NOT depend on `Message.text[256]`. Existing `/cmd`, `/fs`, `/update`
and `/esp` SHALL keep their current behavior.

#### Scenario: Download returns exact bytes

- **WHEN** the web downloads a stored script by name
- **THEN** the response body is byte-identical to the uploaded container

#### Scenario: Upload then run

- **WHEN** the web uploads a valid container and requests execution
- **THEN** the device verifies, stores and runs it

#### Scenario: Oversized payload is rejected

- **WHEN** an upload exceeds the configured script or total size limit
- **THEN** the device rejects it without changing the store

#### Scenario: Unauthorized request is rejected

- **WHEN** a request arrives without valid credentials while auth is enabled
- **THEN** the device returns 401 and does not modify state

### Requirement: Cooperative execution budget

The engine SHALL execute at most a configured instruction count or time budget
per context per loop iteration, then yield. `wait` SHALL suspend a context
without blocking. The engine SHALL NOT exceed the global update budget.

#### Scenario: Long loop yields

- **WHEN** a script runs a loop without `wait`
- **THEN** the engine yields after the per-context budget and other contexts run

#### Scenario: Wait resumes after deadline

- **WHEN** the wait deadline has not elapsed
- **THEN** the context is skipped; after the deadline it resumes

#### Scenario: Fair round-robin

- **WHEN** several contexts are runnable
- **THEN** each receives budget in turn without starving the others

### Requirement: Event dispatch and handler replacement

The engine SHALL register handlers from the container's handler table. When an
event fires, it SHALL run the matching handler in a free context, or log and
drop the event when no context is free. Re-uploading a script SHALL replace its
handlers. Duplicate handlers for the same event in one script SHALL be rejected
at compile time.

#### Scenario: Handler runs with parameters

- **WHEN** an event fires and a handler is registered
- **THEN** the handler runs with the event parameters in `$e0…$e3`

#### Scenario: Re-upload replaces handler

- **WHEN** a script is uploaded again with a changed handler for the same event
- **THEN** the new handler body is used

#### Scenario: No free context drops with log

- **WHEN** an event fires and all contexts are busy
- **THEN** the engine logs the drop and does not corrupt any context

### Requirement: Runtime error reporting

A runtime error SHALL stop the current context and report the script id,
instruction offset, source line when available, and a stable error code. Other
contexts SHALL continue running.

#### Scenario: Invalid data write

- **WHEN** an assignment to `$name` fails in the data provider
- **THEN** the context stops with a data-write error and position

#### Scenario: Error does not stop other scripts

- **WHEN** one context reports a runtime error
- **THEN** other active contexts keep running

### Requirement: Additive rollout

The bytecode engine SHALL be disabled by default and SHALL NOT change the
behavior of the existing text engine. Enabling it SHALL be a per-build or
per-board decision.

#### Scenario: Default build uses text engine

- **WHEN** the bytecode feature flag is disabled
- **THEN** existing text scripts and `/cmd` behave as before

#### Scenario: Enabling bytecode does not alter text behavior

- **WHEN** the bytecode engine is enabled
- **THEN** a text script submitted through the legacy path still runs on the
  text engine unless the id is explicitly registered as bytecode
